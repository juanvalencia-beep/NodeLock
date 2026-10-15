# NodeLock — Explicación del código (Fase 1)

Este documento explica, a nivel universitario, cómo está construido NodeLock: qué hace cada
clase, cómo se representa el grafo, cómo funcionan la recursividad y el backtracking, cómo
y cómo se cuentan los 389.112 patrones.

---

## 1. Visión general de la arquitectura

```
                 +-------------------+
   main.cpp ---> |     ConsoleUI     |   (única capa que usa cin/cout)
                 +---------+---------+
                           | usa
                 +---------v---------+
                 |  NodeLockEngine   |   (fachada: crea y conecta todo)
                 +---------+---------+
        +---------+--------+---------+-----------+
        v         v        v         v           v
  PatternGraph  Pattern  Validator  Enumerator  Simulator / Analyzer
```

Hay dos capas:

1. **Motor** (`include/nodelock/` y `src/core/`): lógica pura. No imprime nada ni lee del
   teclado. Recibe datos y devuelve resultados (o lanza excepciones).
2. **Interfaz** (`src/app/`): `ConsoleUI` pide datos al usuario, llama al motor y formatea
   la salida.

¿Por qué separarlo? Porque en la Fase 2 la interfaz será una página web. Si el motor
imprimiera en consola, habría que reescribirlo. Así, la web solo tendrá que llamar a
`NodeLockEngine` igual que hoy lo hace la consola.

Todas las clases están dentro del `namespace nodelock` para evitar choques de nombres.

---

## 2. Cómo se representan los nodos (`Node.h`)

```cpp
struct Node {
    int id;   // 1..9
    int row;  // 0..2
    int col;  // 0..2
};
```

Los nodos siguen la distribución de un teclado telefónico:

```
id:             (row,col):
1 2 3           (0,0) (0,1) (0,2)
4 5 6           (1,0) (1,1) (1,2)
7 8 9           (2,0) (2,1) (2,2)
```

La relación entre id y coordenadas es: `id = row * 3 + col + 1`. Por ejemplo, el nodo 6
está en la fila 1, columna 2: `1*3 + 2 + 1 = 6`.

Se usa un `struct` (no una clase) porque es un dato simple sin invariantes que proteger.
En el mismo archivo están las constantes globales del problema (`kGridSize = 3`,
`kNodeCount = 9`, `kMinPatternLength = 4`, `kMaxPatternLength = 9`), declaradas
`constexpr` para que el compilador las conozca en tiempo de compilación y no haya
"números mágicos" repartidos por el código.

---

## 3. Cómo se construye y usa el grafo (`PatternGraph`)

### 3.1 Idea

- **Vértices:** los 9 nodos.
- **Aristas:** desde cualquier nodo se puede ir a cualquier otro (grafo completo K9),
  **pero** algunas aristas son *condicionales*. La línea 1→3 pasa físicamente por encima
  del 2, así que solo se permite si el 2 ya fue visitado.

### 3.2 Estructura de datos: matriz de adyacencia con "nodo intermedio"

```cpp
std::vector<Node> nodes_;                       // los 9 nodos
std::array<std::array<int, 10>, 10> between_;   // matriz 10x10 (índice 0 sin usar)
```

- `between_[a][b] == 0` → arista directa: siempre se puede ir de `a` a `b`.
- `between_[a][b] == k` → arista condicional: se necesita haber visitado `k`.

Usamos índices 1..9 directamente (por eso tamaño 10) para que el código se lea igual
que el enunciado: `between_[1][3] == 2`.

Se eligió una **matriz** en lugar de listas de adyacencia porque el grafo es pequeño
(9 nodos) y completo: la matriz da acceso O(1) a "¿qué nodo hay entre a y b?", que es la
pregunta que se hace millones de veces durante el backtracking.

### 3.3 Construcción (constructor de `PatternGraph`)

1. Recorre filas y columnas y crea los 9 nodos con sus coordenadas.
2. Para cada par ordenado (a, b) calcula `dr = b.row - a.row` y `dc = b.col - a.col`.
   En una cuadrícula 3×3, hay un nodo exactamente en medio **si y solo si** `dr` y `dc`
   son ambos pares (0 o ±2). El nodo intermedio está en el punto medio
   `((ra+rb)/2, (ca+cb)/2)`.

Ejemplos:

| Movimiento | dr, dc | ¿Ambos pares? | Intermedio |
|-----------|--------|---------------|-----------|
| 1→3 | 0, 2 | sí | (0,1) = 2 |
| 1→9 | 2, 2 | sí | (1,1) = 5 |
| 4→6 | 0, 2 | sí | (1,1) = 5 |
| 1→2 | 0, 1 | no | — (adyacente) |
| 1→6 | 1, 2 | no | — (salto de caballo) |

La matriz **se calcula** a partir de la geometría, no se escribe a mano. Esto evita errores
de tipeo y muestra que entendemos la regla. Resultado: hay 16 aristas condicionales
dirigidas (8 pares no dirigidos: 1-3, 4-6, 7-9, 1-7, 2-8, 3-9, 1-9, 3-7).

### 3.4 Funciones

| Función | Qué hace |
|---------|----------|
| `nodes()` | Devuelve los 9 nodos. |
| `node(id)` | Devuelve un nodo; lanza `std::out_of_range` si el id no está en 1..9. |
| `isValidId(id)` | `true` si 1 ≤ id ≤ 9. Es `static` porque no depende del objeto. |
| `intermediate(a, b)` | Consulta la matriz: nodo entre `a` y `b`, o 0. |
| `canMove(from, to, visited)` | **La regla central**: `to` no visitado y (sin intermedio o intermedio ya visitado). |
| `directNeighbors(id)` / `conditionalNeighbors(id)` | Listas para mostrar el grafo en consola. |

`VisitedSet` es un alias de `std::array<bool, 10>`: un arreglo de banderas "¿ya visité el
nodo i?". Consultarlo es O(1).

---

## 4. La clase `Pattern`

Representa **solo** una secuencia de ids (`std::vector<int>`). No sabe si es válida: esa
responsabilidad es del validador (principio de responsabilidad única).

- `Pattern::parse(texto)`: método estático "fábrica". Cada dígito es un nodo; espacios,
  `-`, `,` y `>` son separadores. Acepta `1-2-3-6`, `1 2 3 6`, `1236`. Si encuentra otro
  carácter, o si no hay ningún dígito, lanza `std::invalid_argument` con un mensaje claro.
- `toString()`: produce `"1-2-3-6"`.
- `operator==`: compara dos patrones (se usa en el simulador y en las pruebas).

Nota: `parse("0123")` **no** falla; produce el nodo 0, y es el validador quien explica
"Node 0 does not exist". Así cada clase hace una sola cosa.

---

## 5. Validación (`PatternValidator`)

### 5.1 Reglas

1. Longitud entre 4 y 9.
2. Cada id en 1..9.
3. Ningún nodo repetido.
4. No se puede pasar por encima de un nodo no visitado.

Decisión documentada: Android, al dibujar 1→3, *inserta automáticamente* el 2. Nosotros
**no** lo insertamos: el usuario debe escribir `1-2-3`. Así el validador y el enumerador
usan exactamente la misma regla y ambos dan 389.112.

### 5.2 Algoritmo de `validate()`

Simula un dedo recorriendo el patrón, con un `VisitedSet`:

```
para cada posición i:
    actual = nodos[i]
    si actual no está en 1..9      -> error NodeOutOfRange, continuar
    si actual ya fue visitado      -> error RepeatedNode, continuar
    si i > 0:
        medio = intermedio(anterior, actual)
        si medio != 0 y no visitado -> error SkipsUnvisitedNode
    marcar actual como visitado
```

No se detiene en el primer error: los acumula todos en `ValidationResult::issues`
(un `std::vector<ValidationIssue>`), cada uno con un código (`enum class ValidationError`)
y un mensaje legible. `valid()` devuelve `true` si la lista está vacía.

Ejemplo real: `1-3-3` produce tres errores a la vez (corto, salta el 2, repite el 3).

`enum class` (en vez de `enum`) evita conversiones implícitas a `int` y choques de nombres.

---

## 6. Recursividad y backtracking (`PatternEnumerator`)

### 6.1 ¿Qué es el backtracking?

Es una búsqueda en profundidad (DFS) sobre el **árbol de decisiones**. En cada paso:

1. **Elegir** un candidato válido (añadirlo al camino).
2. **Explorar** recursivamente a partir de esa elección.
3. **Deshacer** la elección (quitarla del camino) y probar el siguiente candidato.

Si un candidato viola las reglas, se **poda**: no se explora esa rama, ni ninguna de sus
descendientes. Eso es lo que lo hace más eficiente que generar las 985.824 permutaciones
de 4 a 9 nodos y filtrarlas después.

### 6.2 El código clave (`countRecursive`)

```cpp
void PatternEnumerator::countRecursive(std::vector<int>& path, VisitedSet& visited,
                                       EnumerationStats& stats) const {
    ++stats.recursiveCalls;
    const std::size_t depth = path.size();

    // Every path with 4 or more nodes is a valid pattern.
    if (depth >= static_cast<std::size_t>(kMinPatternLength)) {
        ++stats.countsByLength[depth];
    }
    if (depth == static_cast<std::size_t>(kMaxPatternLength)) return;  // base case

    const int last = path.back();
    for (int next = 1; next <= kNodeCount; ++next) {
        if (!graph_.canMove(last, next, visited)) {
            if (!visited[next]) ++stats.prunedMoves;
            continue;  // prune
        }
        path.push_back(next);  // choose
        visited[next] = true;
        countRecursive(path, visited, stats);  // explore
        visited[next] = false;  // backtrack
        path.pop_back();
    }
}
```

Elementos de la recursividad:

- **Caso base:** `depth == 9` (no hay más nodos libres). También es caso base implícito
  cuando ningún `next` pasa `canMove`: el bucle termina sin llamadas recursivas.
- **Caso recursivo:** para cada vecino permitido, llamarse a sí misma con un camino un nodo
  más largo.
- **Progreso hacia el caso base:** cada llamada añade un nodo, y como no se repiten nodos,
  la profundidad nunca supera 9. La recursión siempre termina.
- **Estado compartido:** `path` y `visited` se pasan **por referencia** y se restauran al
  volver. Esa restauración (`pop_back` y `visited[next] = false`) es literalmente el
  "backtrack". Si se olvidara, el siguiente candidato vería un estado incorrecto.

La función pública `countAll()` lanza la recursión desde cada uno de los 9 nodos iniciales.

### 6.3 Cómo se cuentan las combinaciones

Truco importante: **todo prefijo de un patrón válido también es un camino válido**. Por eso
no hace falta una búsqueda distinta por cada longitud: en un único recorrido del árbol,
cada vez que el camino tiene longitud ≥ 4 se suma uno en `countsByLength[longitud]`.

Resultado verificado:

| Longitud | Patrones |
|---------:|---------:|
| 4 | 1.624 |
| 5 | 7.152 |
| 6 | 26.016 |
| 7 | 72.912 |
| 8 | 140.704 |
| 9 | 140.704 |
| **Total** | **389.112** |

Estadísticas que imprime el programa (y cómo explicarlas):

- **389.497 llamadas recursivas** = 9 (caminos de 1 nodo) + 56 (de 2) + 320 (de 3)
  + 389.112 (patrones completos de 4 a 9). Cada nodo del árbol es una llamada.
  - ¿Por qué 56 caminos de 2 nodos? Desde una esquina hay 5 movimientos directos, desde un
    borde 7 y desde el centro 8: `4·5 + 4·7 + 8 = 56`.
- **14.200 movimientos podados** por la regla del nodo intermedio.

### 6.4 Visitar patrones uno a uno (`forEachPatternOfLength`)

Para el simulador necesitamos *ver* cada patrón, no solo contarlo. `visitRecursive` es la
misma DFS pero:

- El caso base es `path.size() == longitudObjetivo`: entonces llama al **visitor**
  (una `std::function<bool(const std::vector<int>&)>`, normalmente una lambda).
- Si el visitor devuelve `false`, la recursión se detiene y ese `false` se **propaga hacia
  arriba** por todas las llamadas (parada temprana).

Como los candidatos se prueban en orden 1..9, los patrones de una longitud salen en **orden
lexicográfico**: `1-2-3-4`, `1-2-3-5`, `1-2-3-6`, `1-2-3-8`, `1-2-4-3`, …

---

## 7. Recorrido detallado con un patrón de ejemplo: `1-2-3-6`

### 7.1 Validación de `2-1-3-6` (caso con nodo intermedio)

| Paso | Nodo | Anterior | Intermedio | ¿Visitado el intermedio? | Visitados después |
|-----:|-----:|---------:|-----------:|--------------------------|-------------------|
| 0 | 2 | — | — | — | {2} |
| 1 | 1 | 2 | 0 (adyacentes) | — | {1,2} |
| 2 | 3 | 1 | **2** | **sí** → permitido | {1,2,3} |
| 3 | 6 | 3 | 0 | — | {1,2,3,6} |

Longitud 4, sin errores → **VÁLIDO**. En cambio `1-3-6-9` falla en el paso 1: el 2 no
estaba visitado (mensaje: "Move 1 -> 3 passes over node 2…").

### 7.2 Backtracking que encuentra `1-2-3-6` (longitud 4, orden "shortest-first")

```
[1]                     elegir 1 como inicio
 ├─ next=1  visitado -> saltar
 ├─ next=2  canMove(1,2)=sí -> [1,2]
 │   ├─ next=1,2 visitados -> saltar
 │   ├─ next=3  canMove(2,3)=sí -> [1,2,3]
 │   │   ├─ next=1,2,3 visitados
 │   │   ├─ next=4  (3->4 salto de caballo, sin intermedio) -> [1,2,3,4]  intento #1
 │   │   │          longitud 4 = objetivo -> visitor -> pop 4 (backtrack)
 │   │   ├─ next=5  -> [1,2,3,5]  intento #2 -> pop
 │   │   ├─ next=6  -> [1,2,3,6]  intento #3  ¡ENCONTRADO!
 │   │   ├─ next=7  3->7 pasa por 5, no visitado -> PODA
 │   │   ├─ next=8  -> [1,2,3,8]  intento #4
 │   │   └─ next=9  3->9 pasa por 6, no visitado -> PODA
 │   │   (fin del bucle: pop 3, volver a [1,2])
 │   ├─ next=4  -> [1,2,4] ... -> [1,2,4,3] intento #5 ...
```

Así, en el orden *shortest-first* `1-2-3-6` es el **intento 3**. En el orden
*longest-first* primero se recorren todos los de longitud 9, 8, 7, 6 y 5
(140.704 + 140.704 + 72.912 + 26.016 + 7.152 = 387.488) y luego, dentro de los de
longitud 4, vuelve a ser el tercero: 387.488 + 3 = **intento 387.491**. Mismo patrón,
número radicalmente distinto: el número de intentos depende del **orden**, no de una
propiedad "de seguridad" del patrón.

### 7.3 Análisis de `1-2-3-6`

- 4 nodos, 3 segmentos, 3 movimientos rectos, 1 cambio de dirección (forma de "L").
- Distancia total 3,00; empieza en esquina; sin cruces ni pasos por encima.
- 1.624 patrones de longitud 4 → ≈ 10,67 bits *si se eligieran al azar* (no es así).
- Puntuación estructural 5/100 → "Low". Observaciones: empieza en esquina, forma simple.

---

## 8. Simulación de búsqueda (`SearchSimulator`)

- `run(objetivo, orden)`: valida el objetivo (si es inválido lanza
  `std::invalid_argument`), recorre las longitudes en el orden elegido, y para cada
  candidato generado por el enumerador incrementa `attempts` hasta encontrarlo.
  Sigue contando hasta el final para informar del tamaño total del espacio (389.112).
- Órdenes documentados (`enum class SearchOrder`):
  - `ShortestFirst`: longitudes 4→9, lexicográfico dentro de cada una.
  - `LongestFirst`: longitudes 9→4, lexicográfico dentro de cada una.
- `firstAttempts(orden, n)`: devuelve los `n` primeros candidatos (para mostrar el proceso).

**Es una simulación local:** solo compara vectores en memoria. No hay red, ni dispositivos,
ni cuentas.

---

## 9. Análisis (`PatternAnalyzer`)

Primero valida; si el patrón es inválido lanza excepción (no tiene sentido analizarlo).
Luego calcula, segmento a segmento:

| Métrica | Cómo se calcula |
|---------|-----------------|
| Distancia total | Suma de `sqrt(dr² + dc²)` de cada segmento. |
| Tipo de movimiento | Recto si `dr==0` o `dc==0`; diagonal si `|dr|==|dc|`; si no, "caballo" (1,2). |
| Cambios de dirección | Dirección = `(dr, dc)` dividido por su `gcd`; así 1→2 y 1→3 tienen la misma dirección. Se cuenta cada cambio entre segmentos consecutivos. |
| Cruces | Pares de segmentos no consecutivos que se cortan en un punto interior de ambos. Se usa el test de **orientación** (producto cruzado): los extremos de cada segmento quedan en lados opuestos del otro. |
| Pasos por encima | Movimientos con nodo intermedio (que, al ser válido el patrón, ya estaba visitado). |
| Posición inicial | Esquina, borde o centro. |
| Caja envolvente | Ancho y alto ocupados (1 a 3). |
| Espacio de su longitud | `countsByLength[longitud]` y su `log2` en bits. |

**Puntuación heurística (0–100):** cada característica aporta con un **tope**:
longitud (máx. 30), cambios de dirección (25), saltos de caballo (15), cruces (15),
pasos por encima (10), no empezar en esquina (5). Los topes impiden que una sola
característica (en especial la longitud) haga parecer "fuerte" un patrón. Ejemplo
verificado: la "serpiente" `1-2-3-6-5-4-7-8-9` tiene 9 nodos pero no obtiene "High".

**Validez ≠ seguridad.** El programa lo dice explícitamente y muestra las limitaciones:

- La puntuación es una heurística estructural, no una medida de seguridad real.
- Las personas no eligen patrones al azar; un atacante probaría primero formas comunes.
- No se modelan ataques de manchas (*smudge*), observación (*shoulder surfing*) ni los
  bloqueos por intentos fallidos del dispositivo.
- La posición en un orden de enumeración no dice nada sobre otros órdenes.

---

## 10. `NodeLockEngine` (fachada) y gestión de vida de objetos

El validador, el enumerador, el simulador y el analizador guardan **referencias** a otros
componentes (por ejemplo, `const PatternGraph& graph_`). Una referencia no debe sobrevivir
al objeto referido. `NodeLockEngine` resuelve esto siendo el **dueño** de todos:

```cpp
PatternGraph graph_;          // 1º
PatternValidator validator_;  // 2º (usa graph_)
PatternEnumerator enumerator_;// 3º (usa graph_)
EnumerationStats stats_;      // 4º (resultado de enumerator_.countAll())
SearchSimulator simulator_;   // 5º
PatternAnalyzer analyzer_;    // 6º (usa stats_.countsByLength)
```

En C++ los miembros se construyen **en el orden de declaración**, así cada uno se crea
después de aquellos de los que depende. Además, el motor se declara **no copiable**
(`= delete`), porque una copia tendría referencias apuntando al objeto original.

---

## 11. Interfaz de consola (`ConsoleUI`) y `main`

- `ConsoleUI` recibe `std::istream&` y `std::ostream&` en lugar de usar `std::cin` y
  `std::cout` directamente. Gracias a eso las pruebas le pasan un `std::istringstream`
  con entradas simuladas y comprueban la salida en un `std::ostringstream`.
- `run()`: muestra el encabezado *NODELOCK SECURITY* con la cuadrícula y el menú principal
  (analizar un patrón, contar todos los patrones, ver el grafo). Si la entrada termina (EOF)
  sale limpiamente.
- `patternScreen()` y `printPatternReport()`: piden *"Enter your pattern"*, muestran el
  bloque *PATTERN ANALYSIS* (validación, longitud, complejidad heurística, inicio, tipos de
  movimiento, giros y cruces) y un submenú: simulación de búsqueda, otro patrón, análisis
  detallado o volver. Las opciones que necesitan un patrón válido lo comprueban antes.
- `parseValid()`: centraliza el manejo de errores: captura `std::invalid_argument` del
  parser y muestra todos los errores del validador.
- `main.cpp`: sin argumentos abre el menú; con `--grid`, `--count`, `--validate P`,
  `--simulate P`, `--analyze P` ejecuta una sola acción. Devuelve
  códigos de salida: 0 éxito, 1 patrón inválido, 2 uso incorrecto, 3 error inesperado.
  Todo está dentro de un `try/catch` para que ninguna excepción termine el programa sin
  mensaje.

---

## 12. Pruebas (`tests/test_main.cpp`)

Mini-framework propio con macros `CHECK` y `CHECK_THROWS` (sin dependencias externas).
107 comprobaciones en 9 grupos:

1. **graph**: coordenadas, intermedios, vecinos, `canMove`.
2. **parsing**: formatos aceptados y entradas erróneas.
3. **validation**: patrones válidos e inválidos de cada tipo, varios errores a la vez.
4. **backtracking counts**: los 6 conteos por longitud y el total 389.112.
5. **enumeration agrees with validator**: los 389.112 patrones generados pasan el
   validador y ninguno se repite. Son dos implementaciones independientes de las reglas
   que se verifican mutuamente.
6. **enumeration order and early stop**: orden lexicográfico y parada temprana.
7. **search simulation**: intentos exactos en ambos órdenes (1, 3, 1.625, 387.489, …).
8. **pattern analysis**: métricas de patrones conocidos (L, cruces, pasos por encima).
9. **console UI**: sesión simulada completa (menú, patrón inválido, patrón válido, simulación,
   análisis detallado, opción inválida) y EOF.

Compilar y ejecutar:

```bash
mingw32-make test        # Windows (MinGW)  |  make test  en Linux/macOS
```

---

## 13. Complejidad

- Sin reglas, el número de secuencias de 4 a 9 nodos distintos sería
  `P(9,4)+…+P(9,9) = 3.024 + 15.120 + 60.480 + 181.440 + 362.880 + 362.880 = 985.824`.
- Con la poda, el árbol tiene 389.497 nodos. Cada nodo hace un bucle de 9 candidatos con
  `canMove` O(1): trabajo total ≈ 3,5 millones de operaciones simples (unos 20 ms).
- Memoria: O(9) para el camino y el arreglo de visitados (profundidad máxima de recursión 9).
  No se guardan los 389.112 patrones: se generan y se cuentan sobre la marcha.
