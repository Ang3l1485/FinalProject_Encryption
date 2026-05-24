#!/usr/bin/env bash

set -euo pipefail

SIZE_MB="${SIZE_MB:-50}"
RESULTS_DIR="${RESULTS_DIR:-results}"

mkdir -p "${RESULTS_DIR}"

run_strace() {
  local name="$1"
  local mode="$2"
  local output="$3"

  strace -c -o "${RESULTS_DIR}/${name}.strace.txt" \
    ./build/bench_io --mode="${mode}" --size-mb="${SIZE_MB}" --output="${output}" \
    > "${RESULTS_DIR}/${name}.strace.bench.txt"
}

run_time() {
  local name="$1"
  local mode="$2"
  local output="$3"

  /usr/bin/time -v -o "${RESULTS_DIR}/${name}.time.txt" \
    ./build/bench_io --mode="${mode}" --size-mb="${SIZE_MB}" --output="${output}" \
    > "${RESULTS_DIR}/${name}.time.bench.txt"
}

run_all_for() {
  local name="$1"
  local mode="$2"
  local output="$3"

  ./build/bench_io --mode="${mode}" --size-mb="${SIZE_MB}" --output="${output}" \
    | tee "${RESULTS_DIR}/${name}.bench.txt"
  run_strace "${name}" "${mode}" "${output}"
  run_time "${name}" "${mode}" "${output}"
}

bench_value() {
  local name="$1"
  local key="$2"

  awk -F': ' -v wanted="${key}" '$1 == wanted { value = $2 } END { print value }' \
    "${RESULTS_DIR}/${name}.bench.txt"
}

time_value() {
  local name="$1"
  local label="$2"

  awk -F': ' -v wanted="${label}" '{
      key = $1;
      gsub(/^[ \t]+|[ \t]+$/, "", key);
      if (key == wanted) {
        value = $2;
      }
    }
    END { print value }' \
    "${RESULTS_DIR}/${name}.time.txt"
}

elapsed_value() {
  local name="$1"

  awk -F': ' '{
      key = $1;
      gsub(/^[ \t]+|[ \t]+$/, "", key);
      if (key ~ /^Elapsed \(wall clock\) time/) {
        value = $2;
      }
    }
    END { print value }' \
    "${RESULTS_DIR}/${name}.time.txt"
}

elapsed_to_seconds() {
  local raw="$1"

  awk -v raw="${raw}" 'BEGIN {
    parts_count = split(raw, parts, ":");
    if (parts_count == 3) {
      seconds = (parts[1] * 3600) + (parts[2] * 60) + parts[3];
    } else if (parts_count == 2) {
      seconds = (parts[1] * 60) + parts[2];
    } else {
      seconds = raw;
    }
    printf "%.6f", seconds;
  }'
}

bytes_to_mb() {
  local bytes="$1"

  awk -v bytes="${bytes}" 'BEGIN {
    if (bytes == "") {
      printf "n/a";
    } else {
      printf "%.1f MB", bytes / 1024 / 1024;
    }
  }'
}

seconds_to_ms() {
  local seconds="$1"

  awk -v seconds="${seconds}" 'BEGIN {
    if (seconds == "") {
      printf "n/a";
    } else {
      printf "%.1f ms", seconds * 1000;
    }
  }'
}

percent_change() {
  local baseline="$1"
  local candidate="$2"

  awk -v baseline="${baseline}" -v candidate="${candidate}" 'BEGIN {
    if (baseline == "" || baseline + 0 == 0) {
      printf "n/a";
    } else {
      printf "%+.1f%%", ((candidate - baseline) * 100) / baseline;
    }
  }'
}

write_summary() {
  local a_size
  local b_size
  local c_size
  local a_user
  local b_user
  local c_user
  local a_sys
  local b_sys
  local c_sys
  local a_elapsed
  local b_elapsed
  local c_elapsed

  a_size="$(bench_value baseline final_size_bytes)"
  b_size="$(bench_value compressed_write final_size_bytes)"
  c_size="$(bench_value encrypted_write final_size_bytes)"

  a_user="$(time_value baseline "User time (seconds)")"
  b_user="$(time_value compressed_write "User time (seconds)")"
  c_user="$(time_value encrypted_write "User time (seconds)")"

  a_sys="$(time_value baseline "System time (seconds)")"
  b_sys="$(time_value compressed_write "System time (seconds)")"
  c_sys="$(time_value encrypted_write "System time (seconds)")"

  a_elapsed="$(elapsed_to_seconds "$(elapsed_value baseline)")"
  b_elapsed="$(elapsed_to_seconds "$(elapsed_value compressed_write)")"
  c_elapsed="$(elapsed_to_seconds "$(elapsed_value encrypted_write)")"

  cat > "${RESULTS_DIR}/benchmark_summary.md" <<EOF_SUMMARY
# Benchmark summary

| Metrica del Kernel | A. Clasico (Plano directo) | B. Solo Compresion | C. Compresion + Encriptacion | Impacto Final (A vs C) |
|---|---:|---:|---:|---|
| Tamano Transmitido (I/O) | $(bytes_to_mb "${a_size}") | $(bytes_to_mb "${b_size}") | $(bytes_to_mb "${c_size}") | $(percent_change "${a_size}" "${c_size}") (Ahorro de I/O si es negativo) |
| Tiempo de CPU (User Mode) | $(seconds_to_ms "${a_user}") | $(seconds_to_ms "${b_user}") | $(seconds_to_ms "${c_user}") | $(percent_change "${a_user}" "${c_user}") (Costo CPU de comprimir+cifrar) |
| Tiempo de Espera I/O | $(seconds_to_ms "${a_sys}") | $(seconds_to_ms "${b_sys}") | $(seconds_to_ms "${c_sys}") | $(percent_change "${a_sys}" "${c_sys}") (Proxy de latencia kernel/sys) |
| Tiempo Total (Wall-clock) | $(seconds_to_ms "${a_elapsed}") | $(seconds_to_ms "${b_elapsed}") | $(seconds_to_ms "${c_elapsed}") | $(percent_change "${a_elapsed}" "${c_elapsed}") (Resultado final A vs C) |

Escenarios usados:

- A: \`baseline\`, texto plano directo con bloques de 4096 bytes.
- B: \`compressed-write\`, compresion zlib sin cifrado.
- C: \`encrypted-write\`, compresion zlib seguida de cifrado y formato \`.ceio\`.

Evidencia complementaria de backend:

- \`compressed-mmap\` y \`encrypted-mmap\` quedan en \`${RESULTS_DIR}/*mmap*.strace.txt\` y \`${RESULTS_DIR}/*mmap*.time.txt\`.
EOF_SUMMARY
}

# strace -c measures syscall counts and percentages per benchmark scenario.
# /usr/bin/time -v reports real/user/sys time plus memory-related statistics.
run_all_for baseline baseline "${RESULTS_DIR}/plain_${SIZE_MB}mb.txt"
run_all_for compressed_write compressed-write "${RESULTS_DIR}/compressed_write_${SIZE_MB}mb.bin"
run_all_for compressed_mmap compressed-mmap "${RESULTS_DIR}/compressed_mmap_${SIZE_MB}mb.bin"
run_all_for encrypted_write encrypted-write "${RESULTS_DIR}/encrypted_write_${SIZE_MB}mb.ceio"
run_all_for encrypted_mmap encrypted-mmap "${RESULTS_DIR}/encrypted_mmap_${SIZE_MB}mb.ceio"
write_summary
