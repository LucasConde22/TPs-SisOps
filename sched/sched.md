# sched

## Parte 1: Cambio de contexto - De modo kernel a modo usuario

Al realizar un cambio de contexto para trasladarse del modo kernel (ring 0) al modo usuario (ring 3), la función **env_run** realiza un llamado a la función **context_switch** enviándole como parámetro un puntero al trapframe asociado al proceso que se ejecutará:

![image](https://github.com/user-attachments/assets/bc0724a2-4611-49ba-9244-a46362ab3bba)


  Posteriormente, ya habiendo ejecutado el llamado a la función en cuestión, *omitimos* los primeros 4 bytes. Esto se realiza ya que, en **0(%esp)** apunta a la dirección de retorno y, en este caso, no nos interesa:

![image](https://github.com/user-attachments/assets/9be0739b-c07e-49fd-b6cc-d5f4c6727139)

![image](https://github.com/user-attachments/assets/caaaa358-1164-43fd-b18a-61e07e0ea62d)

  Aquí, como se puede apreciar, **esp** ya contiene el primer puntero que nos importa y se puede proseguir con la carga de los valores del trapframe en los registros pertinentes:

![image](https://github.com/user-attachments/assets/d9b77a1d-6ccd-4e78-b058-863ab39e7fb3)

  Un dato relevante es que, como se observa en la imagen, ahora **edi** contiene un *0*, el valor al que apuntaba la dirección que se encontraba contenida en **esp**. Asimismo, esp ahora apunta a la dirección de memoria *4* bytes posterior.

  De esta forma, realizamos sucesivamente el mismo proceso con todos los registros de interés: **esi**; **ebp**; saltamos **esp**; **ebx**; **edx**; **ecx**; **eax**; **es**; y **ds**.

![image](https://github.com/user-attachments/assets/958e8d25-7319-4304-866a-e063770cdfe9)

![image](https://github.com/user-attachments/assets/7d5a1ba7-5874-4032-badf-af490004241c)

![image](https://github.com/user-attachments/assets/71790f10-b4a1-4c11-ac63-c4417279bbfb)

![image](https://github.com/user-attachments/assets/8a516514-264c-4bc2-a0ae-fd56d4cdf265)

![image](https://github.com/user-attachments/assets/a4e28b23-52ff-4abd-b0a4-f6e037652a7c)

![image](https://github.com/user-attachments/assets/a1b63d3e-2528-4006-92cf-db6f17778524)

![image](https://github.com/user-attachments/assets/b053b974-ccf0-4607-a814-689b6604daf7)

![image](https://github.com/user-attachments/assets/73ed49d5-9f4d-4805-9a24-01742905765a)

![image](https://github.com/user-attachments/assets/a981af60-959a-44e6-950b-9907ed3b4105)

  Finalmente, realizamos un salto sobre las direcciones del **trapno** y **traperr**, ya que solo contienen datos que no nos interesa restaurar, y continuamos con la ejecución de iret. Esta restaura los valores faltantes: **cs**; **eip**; y **esp**.

![image](https://github.com/user-attachments/assets/9a514026-7efa-4931-936b-079a32dd98ff)

![image](https://github.com/user-attachments/assets/26d1aaa9-1a11-4a4b-a474-85143018fb0a)

  Cabe aclarar que optamos por la no utilización de *popal* ya que nos parecía relevante mostrar, paso a paso, todas las pequeñas tareas que realiza la función para lograr de forma efectiva el cambio de contexto.

## Parte 3: Scheduler con prioridades

### Idea de la politica

Para la implementación de esta política de scheduling, se buscó diseñar que no fuera tan compleja, como las que utilizan colas o son basadas en calculos probabilisticos, pero que al mismo tiempo ofreciera mejores garantías de equidad y uso eficiente del CPU que un simple Round Robin con prioridades estáticas.

Por este motivo, se tomaron ideas de algunas políticas clásicas como **Round Robin** y **MLFQ** (Multilevel Feedback Queue), incorporando además una mejora propia: El aumento dinámico de prioridades en función de la cantidad de llamadas al scheduler, con el objetivo de prevenir el *starvation*.

### Funcionamiento

#### Inicializacion de entornos con prioridades

Al crear cada entorno, la función `env_create` (ubicada en `/kern/env.c`) se encarga de asignarles una prioridad inicial definida por la constante `INITIAL_PRIORITY`. Esta prioridad representa el nivel más alto que un entorno puede tener al momento de ser creado, lo que garantiza que tendrá más posibilidades de ser ejecutado pronto.

#### Incremento de prioridades

Al ser llamada la función `sched_with_priorities()` se incrementa la variable global `sched_calls` que cuenta la cantidad de invocaciones al scheduler para que, en caso de ser multiplo de la constante `RESTARTING_NUMBER`, se ejecute la función `improve_priorities()` que aumenta la prioridad en 1 a cada proceso en estado `RUNNABLE` y que no haya alcanzado la prioridad inicial. Esto soluciona el posible problema de *starvation*, que ocurre cuando uno o mas entornos quedan indefinidamente sin ser ejecutados.

Por otro lado, la variable global `sched_calls` se utiliza como estadistica al finalizar la ejecucion del scheduler.

#### Selección del proximo entorno

Luego el scheduler busca cuál será el próximo entorno a ejecutar, para eso, inicializa:

+ `next_env` en `NULL` que apuntará al proximo entorno elegido.
+ `highest_priority` en `-1` que guarda la prioridad mas alta encontrada.
+ `start_index` en *0* si no hay un entorno en ejecución, o, el índice siguiente del pereneciente al entorno actual.

A partir de esto, se recorre el arreglo `envs[]` (de tamaño `NENV`) de entornos de manera circular con índice `index` y se evalúan las siguientes condiciones:

+ **Si el entorno no tiene estado `RUNNABLE`:** Se lo saltea.

+ **Si la prioridad del entorno seleccionado es mayor que la variable `highest_priority`:** Se actualiza la variable y también el puntero `next_env` al entorno en el índice `index`.

+ **Si ambos entornos tienen la misma prioridad:** Se desempata por el que menos veces fue ejecutado (`env_runs`), haciendo que la ejecución sea más equitativa. Luego cambia el puntero `next_env` al entorno del índice `index`.

#### Ejecución del entorno elegido

Llegado a este punto pueden suceder dos situaciones:

+ **Existe `next_env` y no es NULL:** En este caso, si aún hay espacio en el historial de ejecuciones `(execution_history)`, se almacena su `env_id` para ser usado proximamente como estadística. A continuación, si la prioridad del entorno es mayor que cero, se le reduce como penalización por haber sido seleccionado, lo cual favorece que otros procesos puedan ser elegidos en futuras rondas. Finalmente, se transfiere el control al entorno mediante la función `env_run(next_env)`.

+ **`next_env` es NULL pero hay un entorno actual corriendo:** En este caso como no se encontró `next_env` pero tiene un entorno que esta corriendo, lo sigue ejecutando con `env_run(curenv)`.

Una vez ejecutado `env_run`, configura el CPU para ejecutar el entorno y el scheduler solo correra nuevamente cuando el entorno actual termine, se bloquee o sea interrumpido.

#### Finalización del scheduler

Llegado a este punto si no se pudo ejecutar ningun entorno elegido, se llama a `sched_halt()` donde termina el scheduler y se detiene la CPU para dejarlo en espera de interrupciones.

### Conclusión

La política de planificación implementada resulta más justa y flexible que un **Round Robin** puro, ya que incorpora un manejo dinámico de prioridades con el objetivo de evitar problemas de *starvation*. Al mismo tiempo, mantiene una estructura sencilla en comparación con otras políticas más complejas que también gestionan prioridades logrando simplicidad.

### Perdida/Ganancia de prioridades

  Para probar el correcto funcionamiento del planificador, desarrollamos dos programas de usuario: *prior.c*, que imprime la prioridad del proceso al momento de su ejecución y cede el procesador, y *test.c*, que simplemente *molesta*: Consumiendo ciclos, imprimiendo, y cediendo el procesador.

  Luego, en *init.c*, configuramos la ejecución de dos procesos que ejecutan *prior.c*, uno de *hello.c* (que simplemete está para *generar una molestia*) y uno de *test.c* para, como mencionamos anteriormente, consumir CPU y molestar el scheduling de los demás environments.

  A la hora de ejecutar *make qemu-nox USE_PR=1*, estas fueron algunos de los mensajes que obtuvimos:

![image](https://github.com/user-attachments/assets/f18e8008-a11c-4602-a3e5-25afcbc701dd)

  Fácilmente se puede notar que el scheduler comienza asignando prioridad máxima, 10, a cada proceso y luego de cada selección la resta en una unidad. También puede verse que cada un determinado número de llamados al scheduler, aumenta la prioridad de los procesos en estado **RUNNABLE** para, como ya explicamos, no se convierta en un *falso* **Round-Robin**.

### Muestreo de estadísticas

  Para este apartado, decidimos mostrar: el total de llamados al scheduler, los primeros 40 procesos planificados y la cantidad total veces que cada proceso fue seleccionado:

![image](https://github.com/user-attachments/assets/0122b029-17de-4ec5-ac08-06de537b0193)

...

![image](https://github.com/user-attachments/assets/ec654225-86fe-4a02-85ca-7aa58f41c3f7)


