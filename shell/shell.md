# shell

### Búsqueda en $PATH

    1) ¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?

        La syscall "execve(2)" permite reemplazar el programa en ejecución por uno nuevo. En esta, los parámetros que toma la función son:
            - pathname: Ruta del binario a ejecutar.
            - argv: Vector de argumentos que recibirá el nuevo programa.
            - envp: Vector de cadenas de forma clave=valor. En este se definen las variables de entorno, junto con sus valores, que tendrá el nuevo programa.

        En cambio, la familia de wrappers "exec(3)" ofrece una diversa variedad de formas de realizar la misma tarea (llamando internamente a execve) de una forma "más cómoda" para el programador:
            - Algunas variantes, que contienen 'p' en su nombre, permiten especificar solo el nombre del binario al ejecutar. En estas, el wrapper se encarga de buscar la ruta completa del mismo en la variable de entorno $PATH.
            - Otras variantes, que contienen 'l' en su nombre, permiten pasar los argumentos mediante una lista de parámetros, en lugar de un vector.
            - Por último, las variantes que contienen 'e' en su nombre ofrecen el seteo de variables de entorno en el nuevo programa. Las que no, hacen que el nuevo programa herede las variables del programa actual.

    2) ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?

        Si, la llamada a exec(3) puede fallar. En nuestra implementación se le informa al usuario que el comando que ingresó no fue encontrado.

### Procesos en segundo plano

    1) ¿Por qué es necesario el uso de señales?

        El uso de señales es necesario puesto que permiten conocer cuando un proceso finaliza su ejecución. De esta forma, cuando el mismo ejecuta la syscall exit(), envía una señal que podemos capturar desde un proceso padre (que recibe SIGCHLD) y ejecutar algún evento en particular mediante un handler. Usando como ejemplo la shell desarrollada en este T.P, luego de recibir la señal se le informa al usuario qué proceso ha finalizado.
    
### Flujo estándar

    1) Investigar el significado de 2>&1, explicar cómo funciona su forma general.

        2>&1 es una expresión que permite rediriir la salida de error estándar (stderr) hacia la misma ubicación que la salida estándar (stdout). En esta:

        2>: Indica que se desean redirigir los errores que, por defecto, se imprimen en stderr (por consola) en una determinada ubicación que se debe especificar a la derecha del mismo. El 2 referencia el file descriptor utilizado para referenciar a stderr, mientras que > siginifica "redirigir a".

        &1 permite especificar un file descriptor específico, en lugar de tener que ingresar la ruta de un archivo. El & referencia a que el contenido a su derecha es un file descriptor y 1, como es sabido, es el file descriptor que referencia a stdout.

    2) Mostrar qué sucede con la salida de cat out.txt en el ejemplo.

        Habiendo ejecutado el comando del ejemplo, "ls -C /home /noexiste >out.txt 2>&1". Luego, al ejecutar "cat out.txt", esta es la salida que recibimos:

            ls: no se puede acceder a '/noexiste': No existe el archivo o el directorio
            /home:
            lucas

        Esto quiere decir dos cosas:
            I. Efectivamente se redirigió stdout al archivo especificado (out.txt).
            II. stderr se redirigió a stdout que, a su vez, ya había sido redirigida a un out.txt y, por lo tanto, ambas salidas terminaron en el mismo archivo.

    3) Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).

        Habiendo ejecutado "ls -C /home /noexiste 2>&1 >out.txt". Luego, al ejecutar "cat out.txt", esta es la salida que recibimos:

            ls: no se puede acceder a '/noexiste': No existe el archivo o el directorio
            /home:
            lucas

        Como se puede observar, la salida resulta ser exactamente la misma que la recibida al ejecutar el comando como lo hicimos anteriormente. Esto se debe a que el parseo de las diferentes partes del comando en nuestra shell no discrimina el orden en el que se realizan las redirecciones y procesa la redirección de stdout previamente a la redirección de stderr.

        En cambio, al ejecutar los mismos comando en bash(1) podimos observar que los errores se imprimieron por pantalla (ya que todavía no se había redirigido stdout) y luego se redirige stdout, por lo que la parte "válida" de nuestro comando si escribió su salida en el archivo out.txt.
        Esto nos lleva a concluir que, a diferencia de nuestra shell, bash si discrimina el orden en que se especifican las redirecciones.

### Tuberías múltiples

    1) Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe. 
        - Al ejecutar un comando como: 

            ls | wc

        y posteriormente ingresar:
        
            echo $?
        
        La shell nos devuelve 0. Esto es debido a que cuando se realiza un pipe, este exit code sera
        el que devuelva wc, que seria el ultimo comando, puede ser 0 si fue exitoso o distinto de 0
        si no.
        ¿Cambia en algo? Si, ya que, en el caso de no correr con el pipe, me devolveria el exit
        code del comando ejecutado.

    2) ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.
        -Si algunos de los comandos falla, por ejemplo:

            echo Hola | no_existe | wc 

        Se muestra en la terminal en bash:

            no_existe: command not found
                0       0       0
        
        Nuestra implementacion por otro lado, muestra en la salida de la terminal:

            Command 'no_existe' not found
                0       0       0
        
        Con esto podemos concluir que, aunque falle un pipe los demas se ejecutaran de igual forma, dado que se implementan con dos forks y conexiones entre procesos hijos y padres es como ocurre esto. Al ejecutarse el comando de la izquierda, detecta al parsear que tambien tiene que ejecutar un proceso derecho (ademas del que ya estaba antes). Se ejecuta ´echo Hola´ y se lo manda a ´no_existe´ el cual no es un comando valido.
        Aun asi, el proceso "anterior" derecho (en este caso ´wc´) de igual forma se va a ejecutar con lo que sea enviado desde ´no_existe´ lo que causa que wc no reciba absolutamente nada.
---

### Variables de entorno temporarias
    1) ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?
        -Esto es necesario dado que al nosotros utilizar setenv(3), que añade una variable de entorno, no debemos crear estas variables en el proceso padre o shell, queremos que sean propias del proceso en cuestion que queremos ejecutar.
    2) ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
        -El comportamiento seria el mismo, sabemos que las variantes que contienen 'e' en su nombre ofrecen el seteo de variables de entorno en el nuevo programa. Con esto en cuenta, es evidente que utilizando esto podemos conseguir el mismo resultado.
        Deberiamos de preparar un arreglo con las variables de entorno que nos pasan para el nuevo proceso, luego de hacer eso, debemos cambiar la llamada en ´exec.c´ de execvp a execve.
---

### Pseudo-variables
    Investigar al menos otras tres variables mágicas estándar, y describir su propósito
    - $0 : Contiene el nombre del script o programa en ejecución. Por ejemplo:

        $ echo "este es el programa $0"

        este es el programa /bin/bash
    - $_ : Contiene el último argumento del último comando ejecutado. Por ejemplo:
    
        $ echo Hola Mundo 

        $ echo $_

        Mundo
    -$! : Devuelve el PID del último proceso ejecutado en segundo plano. Por ejemplo:

        $ sleep 10 &

        $ echo $!

        [En nuestro caso] 10235

---

### Comandos built-in

    1) ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in?

        Si, el comando pwd puede implementarse sin necesidad de ser built-in. De hecho, en cualquier distribución estándar de Linux, su binario ya viene implementado y puede ejecutarse como cualquier otro. Esto es posible porque su tarea es simplemente la de mostrar el directorio actual, sin necesidad de realizar cambios ni de mantener estos luego de su ejecución.
        El motivo de hacerlo como built-in es simplemente por razones de eficiencia y comodidad.

        En cambio, cd no puede implementarse como un binario. Esto se debe a que, durante su ejecución, debe cambiar el directorio actual. De esta forma, si fuese un binario donde para ejecutarlo se llama a fork() previamente desde la shell, se podría cambiar el directorio actual pero una vez finalizada la ejecución del mismo, al volver a la shell, se volvería a trabajar con el directorio anterior. Al ser un built-in, cd puede llamar a chdir() desde el mismo proceso en que se ejecuta la shell, permitiendo así realizar el cambio de manera efectiva.

### Historial

---
