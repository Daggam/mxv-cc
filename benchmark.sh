#!/usr/bin/env bash
set -Eeuo pipefail

# Ejecutar desde la raíz del proyecto, donde se encuentran el Makefile y este script.
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
export LC_ALL=C

usage() {
    cat <<'USAGE'
Uso: ./benchmark.sh [parallel|sequential|all]

  parallel    Ejecuta solo make p y escribe resultados_paralelo.log
  sequential  Ejecuta solo make s y escribe resultados_secuencial.log
  all         Ejecuta ambos algoritmos (opción por defecto)

Opciones mediante variables de entorno:
  REPETITIONS=10  CORES_LIST="1 2 4 8 16 32 64 96"
  MAX_CASES=47    RESET=0   OUT_DIR=resultados
USAGE
}

if (($# > 1)); then
    usage >&2
    exit 2
fi

RUN_S=0
RUN_P=0
case "${1:-all}" in
    parallel) RUN_P=1 ;;
    sequential) RUN_S=1 ;;
    all) RUN_S=1; RUN_P=1 ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'Modo desconocido: %s\n' "$1" >&2; usage >&2; exit 2 ;;
esac

REPETITIONS="${REPETITIONS:-10}"
CORES_LIST="${CORES_LIST:-1 2 4 8 16 32 64 96}"
MAX_CASES="${MAX_CASES:-47}"
RESET="${RESET:-0}"
OUT_DIR="${OUT_DIR:-resultados}"

for config in "REPETITIONS=$REPETITIONS" "MAX_CASES=$MAX_CASES"; do
    value="${config#*=}"
    if [[ ! "$value" =~ ^[1-9][0-9]*$ ]]; then
        printf 'Configuración inválida: %s (debe ser un entero positivo)\n' "$config" >&2
        exit 2
    fi
done
REPETITIONS=$((10#$REPETITIONS))
MAX_CASES=$((10#$MAX_CASES))

if [[ "$RESET" != 0 && "$RESET" != 1 ]]; then
    printf 'RESET debe ser 0 o 1\n' >&2
    exit 2
fi

read -r -a CORES <<< "$CORES_LIST"
if ((${#CORES[@]} == 0)); then
    printf 'CORES_LIST no puede estar vacío\n' >&2
    exit 2
fi
for core in "${CORES[@]}"; do
    if [[ ! "$core" =~ ^[1-9][0-9]*$ ]]; then
        printf 'Core inválido: %s\n' "$core" >&2
        exit 2
    fi
done

if [[ ! -f Makefile && ! -f makefile && ! -f GNUmakefile ]]; then
    printf 'No se encontró un Makefile en %s\n' "$PWD" >&2
    exit 2
fi

mkdir -p -- "$OUT_DIR"
LOG_S="$OUT_DIR/resultados_secuencial.log"
LOG_P="$OUT_DIR/resultados_paralelo.log"
# Solo creamos o reiniciamos los logs del algoritmo seleccionado.
if ((RUN_S)); then
    if [[ "$RESET" == 1 ]]; then : > "$LOG_S"; else touch -- "$LOG_S"; fi
fi
if ((RUN_P)); then
    if [[ "$RESET" == 1 ]]; then : > "$LOG_P"; else touch -- "$LOG_P"; fi
fi

# Reanudación: cada medición se identifica unívocamente por TEST_ID + CORES + ITER.
declare -A DONE_S=()
declare -A DONE_P=()
load_completed() {
    local file="$1" algorithm="$2"
    local line test_id cores iteration key
    while IFS= read -r line || [[ -n "$line" ]]; do
        [[ "$line" == RESULT,* ]] || continue
        [[ "$line" =~ (^|,)TEST_ID=([^,]+) ]] || continue
        test_id="${BASH_REMATCH[2]}"
        [[ "$line" =~ (^|,)CORES=([^,]+) ]] || continue
        cores="${BASH_REMATCH[2]}"
        [[ "$line" =~ (^|,)ITER=([^,]+) ]] || continue
        iteration="${BASH_REMATCH[2]}"
        key="$test_id|$cores|$iteration"
        if [[ "$algorithm" == s ]]; then
            DONE_S["$key"]=1
        else
            DONE_P["$key"]=1
        fi
    done < "$file"
}
if ((RUN_S)); then load_completed "$LOG_S" s; fi
if ((RUN_P)); then load_completed "$LOG_P" p; fi

# La salida del programa contiene tres tiempos etiquetados, no un número fijo de líneas.
# Conservamos la precisión original; 0.000001 no se convierte en 0.00.
parse_times() {
    local algorithm="$1"
    awk -F ':' -v algorithm="$algorithm" '
        function trim(s) {
            gsub(/^[[:space:]]+|[[:space:]]+$/, "", s)
            return s
        }
        BEGIN {
            number = "^[0-9]+([.][0-9]+)?([eE][+-]?[0-9]+)?$"
            required = (algorithm == "s") ? "Tiempo requerido (secuencial)" : "Tiempo requerido (paralelizado)"
        }
        NF >= 2 {
            name = trim($1)
            value = trim($2)
            if (name == "Tiempo para poblar la matriz (secuencial)") {
                pm = value; count_pm++
            } else if (name == "Tiempo para poblar el vector (secuencial)") {
                pv = value; count_pv++
            } else if (name == required) {
                proc = value; count_proc++
            }
        }
        END {
            if (count_pm != 1 || count_pv != 1 || count_proc != 1 ||
                pm !~ number || pv !~ number || proc !~ number) {
                exit 1
            }
            print proc "," pm "," pv
        }
    '
}

run_once() {
    local algorithm="$1" test_id="$2" group="$3" n="$4" m="$5"
    local density="$6" rand_n="$7" cores="$8" iteration="$9"
    local target log output timings t_proc t_pm t_pv exit_code
    local -a command

    if [[ "$algorithm" == s ]]; then
        target=s
        log="$LOG_S"
    else
        target=p
        log="$LOG_P"
    fi

    command=(make --silent --no-print-directory "$target"
             "N=$n" "M=$m" "M_DENSITY=$density" "RAND_N=$rand_n")
    if [[ "$algorithm" == p ]]; then
        command+=("CORES=$cores")
    fi

    printf 'Ejecutando %s: %s, %sx%s, densidad=%s, cores=%s, iter=%s/%s\n' \
        "$target" "$test_id" "$n" "$m" "$density" "$cores" "$iteration" "$REPETITIONS" >&2

    if output=$("${command[@]}" 2>&1); then
        :
    else
        exit_code=$?
        printf 'ERROR: make %s falló (código %s) en %s, cores=%s, iter=%s\n%s\n' \
            "$target" "$exit_code" "$test_id" "$cores" "$iteration" "$output" >&2
        return "$exit_code"
    fi

    if timings=$(printf '%s\n' "$output" | parse_times "$algorithm"); then
        IFS=, read -r t_proc t_pm t_pv <<< "$timings"
    else
        printf 'ERROR: no se pudieron extraer los tres tiempos de %s, cores=%s, iter=%s\nSalida:\n%s\n' \
            "$test_id" "$cores" "$iteration" "$output" >&2
        return 1
    fi

    printf -- '----\nRESULT,TEST_ID=%s,GROUP=%s,ITER=%s,N=%s,M=%s,DENSITY=%s,RAND_N=%s,CORES=%s,TIME_PROC=%s,TIME_PM=%s,TIME_PV=%s\n' \
        "$test_id" "$group" "$iteration" "$n" "$m" "$density" "$rand_n" \
        "$cores" "$t_proc" "$t_pm" "$t_pv" >> "$log"
}

case_index=0
executed_s=0
executed_p=0
skipped=0

# Columnas: grupo N M M_DENSITY RAND_N
while read -r group n m density rand_n; do
    [[ -z "${group:-}" || "$group" == \#* ]] && continue
    case_index=$((case_index + 1))
    if ((case_index > MAX_CASES)); then
        break
    fi
    printf -v test_id 'T%03d' "$case_index"

    # Referencia secuencial: 10 repeticiones por configuración, NO por cantidad de cores.
    if ((RUN_S)); then
        for ((iteration = 1; iteration <= REPETITIONS; iteration++)); do
            key="$test_id|1|$iteration"
            if [[ -n "${DONE_S[$key]+x}" ]]; then
                skipped=$((skipped + 1))
                continue
            fi
            run_once s "$test_id" "$group" "$n" "$m" "$density" "$rand_n" 1 "$iteration"
            DONE_S["$key"]=1
            executed_s=$((executed_s + 1))
        done
    fi

    # Versión paralela: 10 repeticiones para cada cantidad de cores.
    if ((RUN_P)); then
        for cores in "${CORES[@]}"; do
            for ((iteration = 1; iteration <= REPETITIONS; iteration++)); do
                key="$test_id|$cores|$iteration"
                if [[ -n "${DONE_P[$key]+x}" ]]; then
                    skipped=$((skipped + 1))
                    continue
                fi
                run_once p "$test_id" "$group" "$n" "$m" "$density" "$rand_n" "$cores" "$iteration"
                DONE_P["$key"]=1
                executed_p=$((executed_p + 1))
            done
        done
    fi
done <<'PARAMS'
# 1. Matrices pequeñas
small 128 128 0.3 100
small 256 256 0.3 100
small 512 512 0.3 100
small 1024 1024 0.3 100
small 2048 2048 0.3 100
small 4096 4096 0.3 100
# 2. Matrices grandes
large 8192 8192 0.3 100
large 16384 16384 0.3 100
large 32768 32768 0.3 100
# 3. Rectangulares: más columnas
wide 1024 4096 0.3 100
wide 2048 8192 0.3 100
wide 4096 16384 0.3 100
wide 8192 32768 0.3 100
wide 16384 65536 0.3 100
# 4. Rectangulares: más filas
tall 4096 1024 0.3 100
tall 8192 2048 0.3 100
tall 16384 4096 0.3 100
tall 32768 8192 0.3 100
tall 65536 16384 0.3 100
# 5. Extremadamente rectangulares: más columnas
extreme_wide 1024 65536 0.3 100
extreme_wide 2048 131072 0.3 100
# 6. Extremadamente rectangulares: más filas
extreme_tall 65536 1024 0.3 100
extreme_tall 131072 2048 0.3 100
# 7. Variación de densidad: matriz mediana
density_medium 4096 4096 0.001 100
density_medium 4096 4096 0.01 100
density_medium 4096 4096 0.05 100
density_medium 4096 4096 0.10 100
density_medium 4096 4096 0.25 100
density_medium 4096 4096 0.50 100
# 8. Variación de densidad: matriz grande
density_large 16384 16384 0.001 100
density_large 16384 16384 0.01 100
density_large 16384 16384 0.05 100
density_large 16384 16384 0.10 100
density_large 16384 16384 0.25 100
density_large 16384 16384 0.50 100
# 9. Variación de densidad: matriz muy grande
density_very_large 32768 32768 0.001 100
density_very_large 32768 32768 0.01 100
density_very_large 32768 32768 0.05 100
density_very_large 32768 32768 0.10 100
density_very_large 32768 32768 0.25 100
density_very_large 32768 32768 0.50 100
# 10. Variación de RAND_N
rand_n 8192 8192 0.3 1
rand_n 8192 8192 0.3 10
rand_n 8192 8192 0.3 1000
rand_n 8192 8192 0.3 10000
rand_n 8192 8192 0.3 100000
PARAMS

printf '\nPruebas finalizadas. Casos: %s | nuevas ejecuciones secuenciales: %s | paralelas: %s | omitidas (ya guardadas): %s\n' \
    "$(( case_index < MAX_CASES ? case_index : MAX_CASES ))" "$executed_s" "$executed_p" "$skipped" >&2
printf 'Archivos actualizados:\n' >&2
if ((RUN_S)); then printf '  %s\n' "$LOG_S" >&2; fi
if ((RUN_P)); then printf '  %s\n' "$LOG_P" >&2; fi
