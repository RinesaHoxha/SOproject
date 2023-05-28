Ky projekt është realizuar nga grupi 34 në lëndën Sisteme Operative.

Detyra e grupit tone ishte te dizajnojme dhe zbatojme një sistem të shpërndarë duke përdorur IPC, threads dhe
sinkronizimi në C në Linux. Pra projekti u be ne gjuhen C ne Sistemin Operativ Ubuntu.


Projekti u punua nga: 
Rinesa Hoxha,
Rinesa Nura, 
Festim Krasniqi,
Gentrit Ademi

Hapat e ekzekutimit:

Kodi i serverit (Serveri.c) dhe kodi i klientit (Klienti.c)  përpilohen veçmas duke përdorur një përpilues C, siç është GCC.

gcc Serveri.c -o Serveri
gcc Klienti.c -o Klienti

Hapen dy dritare të veçanta terminali, njëra për serverin dhe njëra për klientin.

Ekzekutohet programi i serverit në terminalin e serverit:

./server
Kjo do të nisë serverin dhe do ta bëjë atë gati për të trajtuar kërkesat e klientit.

Ekzekutohet programi i klientit në terminalin e klientit:

./klient
Kjo do të nisë programin e klientit dhe do t'ju kërkojë të shkruani një mesazh për ta dërguar në server.
