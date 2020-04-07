# TextChat

This project is a mail server connecting customers in pairs.
In addition, message exchanges between two clients are done in turn: the server therefore successively invites the client either to enter a message or to wait for the message from its correspondent.

The protocol used here is TCP, and the synchronization and concurrency aspects are managed via UNIX threads.

**Guide d’utilisation**

Run the command `make` in the project repository. 

To run the server, run the following command : 
Ex: `./serveurchat`
To run the client, run the following command : 
Ex: `./clientchat <ip_du_serveur> 33016`

Details about this project are available in the Rapport (in french).

