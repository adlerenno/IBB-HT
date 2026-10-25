# Alternative Bäume in der parallelen Konstruktion einer Burrows-Wheeler-Transformation

## Kompilieren

Die Tests und der Performanz-test können mit

	make

kompiliert werden.


## Tests

Die Tests können mit 

	make test

oder manuell durch Ausführen des kompilierten `test_bwt` Programms
ausgeführt werden.


## Benchmark

Bei dem Performanz-Test wird die Kompression mehrfach mit verschiedenen
Anordnungen durchgeführt und die Laufzeit jedes Durchlaufs gemessen.
Nach allen Durchlaufen einer Anordnung werden die durchschnittliche Laufzeit
und das Maximum und Minimum ausgegeben.
Die Parameter des Performanz-Tests sind oben in `benchmark.c` angegeben:

```c
/* Benchmark parameters */
#define RUN_COUNT 10
static struct {
  int alphabet_size;
  int max_depth;
} params[] = {
    {2, 10}, {4, 10}, {8, 9}, {16, 6}, {32, 5}, {64, 4}, {128, 3}, {255, 3},
};
```

Das Programm erwartet einige Eingabedateien per Kommandozeilen-Parameter
zu erhalten:

	Usage: ./benchmark FILE...


## Quellcode dritter

Dieses Projekt verwendet folgende Bibliotheken,
die sich im `deps` Ordner befinden:

- [greatest](https://github.com/silentbicycle/greatest), eine kleine Bibliothek zum Testen
- [Eine Übersetzung von Timsort zu C](https://github.com/patperry/timsort) als Implementierung eines stabilen Sortier-Algorithmus
- [`stb_ds.h`](https://nothings.org/stb_ds/) ([GitHub Repository](https://github.com/nothings/stb/blob/master/stb_ds.h)) für dynamische Arrays
