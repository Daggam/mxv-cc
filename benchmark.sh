#!/usr/bin/env bash

set -uo pipefail

REPETITIONS=10
CORES_VALUES=(1 8 16 64 128)
SEQUENTIAL_LOG="resultados_secuencial.log"
PARALLEL_LOG="resultados_paralelo.log"

# El algoritmo y SECTION pueden indicarse de estas formas:
#   ./benchmark.sh
#   ./benchmark.sh parallel
#   ./benchmark.sh sequential
#   SECTION=1 ./benchmark.sh parallel
#   ./benchmark.sh parallel 1
#   ./benchmark.sh 1
ALGORITHM="both"
SECTION_ARGUMENT=""

case "${1:-}" in
    parallel|sequential)
        ALGORITHM="$1"
        SECTION_ARGUMENT="${2:-}"
        ;;
    both)
        SECTION_ARGUMENT="${2:-}"
        ;;
    "") ;;
    *)
        # Compatibilidad con: ./benchmark.sh 1 o ./benchmark.sh SECTION=1
        SECTION_ARGUMENT="$1"
        ;;
esac

SECTION="${SECTION:-${SECTION_ARGUMENT:-all}}"
SECTION="${SECTION#SECTION=}"

case "$SECTION" in
    1|2|3|4|5|6|7|8|9|10) ;;
    *) SECTION="all" ;;
esac

declare -a SECTION_NAMES=(
    ""
    "Matrices pequeñas"
    "Matrices grandes"
    "Matrices rectangulares (más columnas)"
    "Matrices rectangulares (más filas)"
    "Matrices extremadamente rectangulares (más columnas)"
    "Matrices extremadamente rectangulares (más filas)"
    "Variación de densidad - matriz mediana"
    "Variación de densidad - matriz grande"
    "Variación de densidad - matriz muy grande"
    "Variación de RAND_N"
)

# Formato: sección|N|M|M_DENSITY|RAND_N
declare -a PARAMS=(
    "1|128|128|0.3|100"
    "1|256|256|0.3|100"
    "1|512|512|0.3|100"
    "1|1024|1024|0.3|100"
    "1|2048|2048|0.3|100"
    "1|4096|4096|0.3|100"

    "2|8192|8192|0.3|100"
    "2|16384|16384|0.3|100"
    "2|32768|32768|0.3|100"

    "3|1024|4096|0.3|100"
    "3|2048|8192|0.3|100"
    "3|4096|16384|0.3|100"
    "3|8192|32768|0.3|100"
    "3|16384|65536|0.3|100"

    "4|4096|1024|0.3|100"
    "4|8192|2048|0.3|100"
    "4|16384|4096|0.3|100"
    "4|32768|8192|0.3|100"
    "4|65536|16384|0.3|100"

    "5|1024|65536|0.3|100"
    "5|2048|131072|0.3|100"

    "6|65536|1024|0.3|100"
    "6|131072|2048|0.3|100"

    "7|4096|4096|0.001|100"
    "7|4096|4096|0.01|100"
    "7|4096|4096|0.05|100"
    "7|4096|4096|0.10|100"
    "7|4096|4096|0.25|100"
    "7|4096|4096|0.50|100"

    "8|16384|16384|0.001|100"
    "8|16384|16384|0.01|100"
    "8|16384|16384|0.05|100"
    "8|16384|16384|0.10|100"
    "8|16384|16384|0.25|100"
    "8|16384|16384|0.50|100"

    "9|32768|32768|0.001|100"
    "9|32768|32768|0.01|100"
    "9|32768|32768|0.05|100"
    "9|32768|32768|0.10|100"
    "9|32768|32768|0.25|100"
    "9|32768|32768|0.50|100"

    "10|8192|8192|0.3|1"
    "10|8192|8192|0.3|10"
    "10|8192|8192|0.3|1000"
    "10|8192|8192|0.3|10000"
    "10|8192|8192|0.3|100000"
)

extract_time() {
    local output="$1"
    local pattern="$2"

    awk -v pattern="$pattern" '
        index($0, pattern) {
            value = $NF
            gsub(/\r/, "", value)
            print value
            exit
        }
    ' <<< "$output"
}

run_test() {
    local algorithm="$1"
    local n="$2"
    local m="$3"
    local density="$4"
    local rand_n="$5"
    local cores="$6"
    local log_file="$7"
    local make_target output time_proc time_pm time_pv density_formatted

    if [[ "$algorithm" == "secuencial" ]]; then
        make_target="s"
    else
        make_target="p"
    fi

    if ! output=$(make "$make_target" N="$n" M="$m" M_DENSITY="$density" RAND_N="$rand_n" CORES="$cores" 2>&1); then
        printf 'Error al ejecutar make %s con N=%s, M=%s, M_DENSITY=%s, RAND_N=%s, CORES=%s\n' \
            "$make_target" "$n" "$m" "$density" "$rand_n" "$cores" >&2
        printf '%s\n' "$output" >&2
        exit 1
    fi

    time_pm=$(extract_time "$output" "Tiempo para poblar la matriz")
    time_pv=$(extract_time "$output" "Tiempo para poblar el vector")
    time_proc=$(extract_time "$output" "Tiempo requerido")

    if [[ -z "$time_pm" || -z "$time_pv" || -z "$time_proc" ]]; then
        printf 'Error: no se pudieron extraer todos los tiempos de la salida de make %s.\n' "$make_target" >&2
        printf '%s\n' "$output" >&2
        exit 1
    fi

    printf -v density_formatted '%.4f' "$density"
    printf 'RESULT,N=%s,M=%s,DENSITY=%s,RAND_N=%s,CORES=%s,TIME_PROC=%s,TIME_PM=%s,TIME_PV=%s\n' \
        "$n" "$m" "$density_formatted" "$rand_n" "$cores" \
        "$time_proc" "$time_pm" "$time_pv" >> "$log_file"
}

if [[ "$ALGORITHM" != "parallel" ]]; then
    : > "$SEQUENTIAL_LOG"
fi

if [[ "$ALGORITHM" != "sequential" ]]; then
    : > "$PARALLEL_LOG"
fi

for section_number in {1..10}; do
    if [[ "$SECTION" != "all" && "$SECTION" != "$section_number" ]]; then
        continue
    fi

    section_name="${SECTION_NAMES[$section_number]}"

    if [[ "$ALGORITHM" != "parallel" ]]; then
        printf '%s\n\n' "---- SECCIÓN $section_number: $section_name ----" >> "$SEQUENTIAL_LOG"
    fi

    if [[ "$ALGORITHM" != "sequential" ]]; then
        printf '%s\n\n' "---- SECCIÓN $section_number: $section_name ----" >> "$PARALLEL_LOG"
    fi

    for params in "${PARAMS[@]}"; do
        IFS='|' read -r param_section n m density rand_n <<< "$params"

        if [[ "$param_section" != "$section_number" ]]; then
            continue
        fi

        if [[ "$ALGORITHM" != "parallel" ]]; then
            for ((repetition = 1; repetition <= REPETITIONS; repetition++)); do
                printf 'Sección %s | Secuencial | N=%s M=%s | repetición %s/%s\n' \
                    "$section_number" "$n" "$m" "$repetition" "$REPETITIONS"
                run_test "secuencial" "$n" "$m" "$density" "$rand_n" 1 "$SEQUENTIAL_LOG"
            done
        fi

        if [[ "$ALGORITHM" != "sequential" ]]; then
            for cores in "${CORES_VALUES[@]}"; do
                for ((repetition = 1; repetition <= REPETITIONS; repetition++)); do
                    printf 'Sección %s | Paralelo | N=%s M=%s | cores=%s | repetición %s/%s\n' \
                        "$section_number" "$n" "$m" "$cores" "$repetition" "$REPETITIONS"
                    run_test "paralelo" "$n" "$m" "$density" "$rand_n" "$cores" "$PARALLEL_LOG"
                done
            done
        fi
    done

    if [[ "$ALGORITHM" != "parallel" ]]; then
        printf '\n' >> "$SEQUENTIAL_LOG"
    fi

    if [[ "$ALGORITHM" != "sequential" ]]; then
        printf '\n' >> "$PARALLEL_LOG"
    fi
done

case "$ALGORITHM" in
    sequential)
        printf 'Pruebas finalizadas. Resultados guardados en %s.\n' "$SEQUENTIAL_LOG"
        ;;
    parallel)
        printf 'Pruebas finalizadas. Resultados guardados en %s.\n' "$PARALLEL_LOG"
        ;;
    *)
        printf 'Pruebas finalizadas. Resultados guardados en %s y %s.\n' \
            "$SEQUENTIAL_LOG" "$PARALLEL_LOG"
        ;;
esac
