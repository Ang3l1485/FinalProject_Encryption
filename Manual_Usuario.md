# Manual de Usuario

## Requisitos

El proyecto esta pensado para Linux o WSL porque usa `ncurses`, `mmap`,
`strace` y herramientas POSIX.

Instalar dependencias:

```sh
sudo apt-get update
sudo apt-get install build-essential zlib1g-dev libncurses-dev strace valgrind
```

## Compilacion

Desde la raiz del proyecto:

```sh
make clean && make
```

Ejecutar pruebas:

```sh
make test
```

## Ejecutar el editor

Modo con `write`:

```sh
./build/editor --io=write documento.ceio
```

Modo con `mmap`:

```sh
./build/editor --io=mmap documento.ceio
```

Si el archivo existe, el programa pedira la clave antes de abrirlo. Si el
archivo es nuevo, la clave se pedira al guardar por primera vez.

## Teclas principales

| Tecla | Accion |
|---|---|
| `Ctrl+S` | Guardar archivo. Pide clave y la borra despues de usarla. |
| `Ctrl+Q` | Salir. |
| `F10` | Salida alternativa. |
| `Esc` | Salida alternativa. |
| Flechas | Mover cursor. |
| `Backspace` | Borrar caracter anterior. |
| `Delete` | Borrar caracter actual. |

## Ejemplo de uso

1. Compilar:

```sh
make clean && make
```

2. Crear o abrir un documento:

```sh
./build/editor --io=write documento.ceio
```

3. Escribir texto.

4. Guardar con `Ctrl+S`.

5. Digitar la clave cuando la UI la pida. La clave no se vera en pantalla.

6. Salir con `Ctrl+Q`, `F10` o `Esc`.

7. Volver a abrir el archivo:

```sh
./build/editor --io=write documento.ceio
```

8. Ingresar la misma clave usada para guardar.

## Nota sobre archivos existentes

Si un archivo `.ceio` fue creado antes de la entrada interactiva de clave, puede
haber sido guardado con la clave de prueba usada por versiones anteriores:

```text
default_test_key
```

Para uso normal, cree un archivo nuevo y elija su propia clave desde la UI.

## Benchmark rapido

Compilar el benchmark:

```sh
make build/bench_io
```

Ejecutar un escenario:

```sh
./build/bench_io --mode=baseline --size-mb=50 --output=results/plain_50mb.txt
./build/bench_io --mode=compressed-write --size-mb=50 --output=results/compressed_write_50mb.bin
./build/bench_io --mode=encrypted-write --size-mb=50 --output=results/encrypted_write_50mb.ceio
```

## Benchmark completo con profiling

Ejecutar:

```sh
make profile
```

Archivos generados:

| Archivo | Contenido |
|---|---|
| `results/*.bench.txt` | Salida directa del benchmark. |
| `results/*.strace.txt` | Resumen de llamadas al sistema. |
| `results/*.time.txt` | Tiempo user, sys, wall-clock y memoria. |
| `results/benchmark_summary.md` | Tabla final A/B/C para la sustentacion. |

## Prueba pequena para verificar el script

Para no generar 50 MB durante una prueba rapida:

```sh
SIZE_MB=1 RESULTS_DIR=build/profile-test bash scripts/run_profile.sh
cat build/profile-test/benchmark_summary.md
```

## Modos de benchmark

| Modo | Descripcion |
|---|---|
| `baseline` | Texto plano directo en bloques de 4096 bytes. |
| `compressed-write` | Solo compresion con backend `write`. |
| `compressed-mmap` | Solo compresion con backend `mmap`. |
| `encrypted-write` | Compresion + encriptacion con backend `write`. |
| `encrypted-mmap` | Compresion + encriptacion con backend `mmap`. |

## Tabla esperada

El archivo `results/benchmark_summary.md` queda con esta estructura:

| Metrica del Kernel | A. Clasico (Plano directo) | B. Solo Compresion | C. Compresion + Encriptacion | Impacto Final (A vs C) |
|---|---:|---:|---:|---|
| Tamano Transmitido (I/O) | Resultado A | Resultado B | Resultado C | Cambio porcentual |
| Tiempo de CPU (User Mode) | Resultado A | Resultado B | Resultado C | Overhead de CPU |
| Tiempo de Espera I/O | Resultado A | Resultado B | Resultado C | Ahorro de latencia |
| Tiempo Total (Wall-clock) | Resultado A | Resultado B | Resultado C | Rentabilidad final |

## Validar que el archivo no esta en claro

Despues de generar un `.ceio` cifrado:

```sh
strings results/encrypted_write_50mb.ceio | head
hexdump -C results/encrypted_write_50mb.ceio | head
```

La salida no deberia mostrar el texto original del documento.

## Problemas comunes

| Problema | Solucion |
|---|---|
| `ncurses.h: No such file or directory` | Instalar `libncurses-dev`. |
| `zlib.h: No such file or directory` | Instalar `zlib1g-dev`. |
| `strace: command not found` | Instalar `strace`. |
| El archivo no abre | Verificar que la clave sea la misma usada al guardar. |
| El benchmark tarda mucho | Probar con `SIZE_MB=1` antes de usar 50 MB. |

## Archivos importantes

| Archivo | Uso |
|---|---|
| `reporte.md` | Reporte academico alineado con la rubrica. |
| `Manual_Usuario.md` | Este manual de instalacion, ejecucion y ejemplos. |
| `scripts/run_profile.sh` | Automatiza evidencia de benchmark. |
| `results/benchmark_summary.md` | Tabla final generada por el profiling. |
