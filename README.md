# Planificador Dieciochero

Tarea 1 de Sistemas Operativos. Se debe simular un día de fiestas patrias
donde cada actividad (prender el carbón, comprar la carne, etc) depende de
otras, así que se modela como un grafo (DAG) y cada actividad corre en su
propio proceso.

## Como compilar

	make

se necesita g++ con c++17

## Uso
	
	 ./planificador plan.txt K

plan.txt tiene el formato que pide el enunciado (ID : nombre : tiempo_ms : deps)
y K es cuantos procesos pueden estar corriendo al mismo tiempo.


Ejemplo: 

	./planificador tests/plan_ejemplo.txt 2

Tambien se incluye un plan de prueba mas grande para la parte de estres: 

	./planificador tests/plan_estres_10000.txt 50

## Estructura del código

- `grafo.hpp` / `grafo.cpp`: parsea el `plan.txt` y construye el grafo —
  quién depende de quién, y también la relación inversa (quién depende de
  mí), que el archivo no da directamente pero el planificador necesita.
- `scheduler.hpp` / `scheduler.cpp`: hace `fork()` por cada actividad,
  respeta el límite `K` de concurrencia, y maneja los pipes con los que las
  actividades se avisan entre sí cuando terminan.
- `main.cpp`: arma el grafo y llama al planificador.

## Decisiones de diseño

No se usaron Threads ni mecanismos de sinc. de hilos en ningun punto.

Seguir misma estructura: 


-Christian Pereira
-Benjamin Paredes
