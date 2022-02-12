# TP para la materia Sistemas Operativos de la UTN FRBA :mortar_board:

**Simulación de un sistema distribuido que utiliza el concepto de Colas de Mensajes (o Message Queue).**

Los componentes incluidos dentro de la arquitectura del sistema trabajan en conjunto para la planificación y ejecución de distintas operaciones, entre las que se encuentran, por ejemplo: lectura, escritura y almacenamiento de mensajes recibidos a través de sockets. Las operaciones que conforman estos mensajes están asociadas y vinculadas al mundo de Pokémon.

Los componentes del sistema son:

:mega: Un proceso publisher que ingresa mensajes al sistema (Game Boy).

:mailbox_with_mail: Un proceso administrador de las Colas de Mensajes (Broker). Este proceso utiliza dos sistemas de administración de su memoria interna (caché de mensajes) proporcionados por archivo de configuración: Buddy System y Particiones Dinámicas con compactación.

:arrow_forward: Procesos que obtienen los mensajes y planifican en función de ellos (Team). Este proceso utiliza cuatro algoritmos de planificación proporcionados por archivo de configuración: FIFO, RR, SJF con desalojo y SJF sin desalojo.

:floppy_disk: Procesos filesystem que se encargan de mantener los archivos en el tiempo (Game Card).

:star: El módulo nuestras-commons es una shared-library que provee funciones de manejo de sockets, envíos y recepciones de mensajes comunes a todos los procesos.


Click para ver el [enunciado](https://docs.google.com/document/d/1be91Gn93O2Vp8frZoV1i5CmtOG0scE1PS8dMHsCP314/edit).
