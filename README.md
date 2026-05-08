# Parcial3-SistemasOperativos

Primera etapa del editor de archivos comprimidos en C/Linux.

Esta entrega implementa solamente el nucleo de edicion en memoria. Todavia no
incluye ncurses, compresion, formato `.ceio`, ni persistencia con `write` o
`mmap`.

## Que es un GapBuffer

Un `GapBuffer` es una estructura de texto que mantiene un espacio vacio cerca
del cursor. Insertar caracteres en la posicion actual es eficiente porque se
escribe dentro de ese hueco. Cuando el cursor se mueve, el texto se desplaza de
un lado del hueco al otro.

En este proyecto, `gap_buffer` permite:

- inicializar y liberar memoria,
- insertar un caracter,
- borrar antes del cursor,
- borrar en el cursor,
- mover el cursor a izquierda, derecha o a una posicion especifica,
- obtener longitud logica y posicion del cursor,
- exportar el contenido a un buffer plano,
- cargar contenido desde un buffer plano.

## Que hace editor_core

`editor_core` es una capa simple encima del `GapBuffer`. Su responsabilidad es
representar el nucleo logico del editor, independiente de UI, archivos y
compresion.

`EditorCore` mantiene:

- `GapBuffer buffer`,
- bandera `dirty`,
- estado de inicializacion.

El modulo marca `dirty = 1` cuando el texto cambia, y permite volver a marcarlo
limpio despues de guardar.

## Funciones publicas principales

El siguiente modulo del proyecto debe usar estas funciones:

```c
int editor_core_init(EditorCore *core);
void editor_core_free(EditorCore *core);

int editor_core_insert_char(EditorCore *core, unsigned char ch);
int editor_core_backspace(EditorCore *core);
int editor_core_delete(EditorCore *core);

int editor_core_move_left(EditorCore *core);
int editor_core_move_right(EditorCore *core);
int editor_core_move_to(EditorCore *core, size_t position);

size_t editor_core_get_cursor(const EditorCore *core);
size_t editor_core_get_length(const EditorCore *core);

int editor_core_to_buffer(const EditorCore *core,
                          unsigned char **out_data,
                          size_t *out_size);
int editor_core_load_buffer(EditorCore *core,
                            const unsigned char *data,
                            size_t size);

int editor_core_is_dirty(const EditorCore *core);
void editor_core_mark_clean(EditorCore *core);
```

`editor_core_to_buffer` entrega un buffer plano que debe liberarse con `free`.
`editor_core_load_buffer` reemplaza el contenido actual por bytes ya leidos por
otro modulo.

## Compilar

```sh
make
```

Esto genera:

- `build/editor_core_demo`
- `build/test_editor_core`

## Probar

```sh
make test
```

Validar memoria con Valgrind:

```sh
make valgrind
```

Ejecutar demo temporal:

```sh
./build/editor_core_demo
```

## Pendiente para la siguiente etapa

- UI tipo nano con ncurses.
- Modulo de compresion antes de guardar.
- Formato binario `.ceio`.
- Persistencia con POSIX `write` y `mmap`.
- Profiling con `strace` y `time`.

El contrato importante para el siguiente companero es:

- usar `editor_core_to_buffer` para obtener el texto que luego se comprimira y
  guardara,
- usar `editor_core_load_buffer` para cargar texto descomprimido o leido desde
  otro modulo,
- usar `editor_core_is_dirty` y `editor_core_mark_clean` para controlar cambios
  pendientes.
