Marcu-Nicolescu Cezar-George
335CB


			Tema 2 - Protocoale de comunicatii


	Voi enunta cum am procedat pentru a implementa tema , trebuie sa fiu sincer , a cam fost o provocare sa o fac in c, pentru ca m-am pierdut la un moment dat prin indexi si pozitii in vectorii de structuri. In cod am lasat comentarii , pentru a clarifica de ce am procedat asa.
Am pornit de la structura laboratorului 8 si am adus anumite modificari la structura acestuia pentru a putea adapta laboratorul la cerintele temei, clienti udp si clienti tcp. In primul rand , in functie de ce socket imi era setat in select am 3 cazuri , cel in care comunic cu un client TCP , cel in care primesc mesaje de la UDP si cel in care serverul primeste un input de la tastatura.

	Structurile pe care le-am folosit(helpers.h) sunt de doua tipuri : pentru transmiterea de mesaje (msgServ / msgClient) si pentru prelucrarea de date si gestiunea informatiei (fisa_client, listaTop, store).
	In structura de topicuri am retinut pentru fiecare topic creat o lista de clienti ce sunt abonati la acesta , socketii clientilor, cat si pentru fiecare client SF-ul. Mai am 2 variabile ce ma ajuta sa parcurg vectorul de topicuri, un index ce reprezinta pozitia curenta in vector , si poz ce imi indica pozitia curenta in vectorul de clienti / sf / socketi a topicului actual.
	Aceasta este principala structura ce ma ajuta in comunicare .
	Structura de stocare a mesajelor ce nu au putut fi transmise este formata din mesajul de la server la client, un id al clientului ce este deconectat , un socket al acestuia .

	In momentul in care primesc un mesaj de la UDP imi construiesc mesajul pe care il voi transmite catre clienti , parcurg vectorul de topicuri sa vad daca am in acesta topicul din mesajul de la udp , daca nu, il construiesc , iar in caz afirmativ incep sa transmit mesaje clientilor din lista specifica topicului.	Mai intai verific sa vad daca clientul este conectat (vad daca file descriptorul acestuia este setat in multimea de file descriptori) , daca nu atunci stochez mesajul si astept sa se reconecteze pentru a-i transmite mesajul.

	Atunci cand primesc un mesaj de la un client TCP am cateva posibilitati de mesaje : ori se conecteaza pentru prima oara , cazul in care nu il am in vectorul de clienti (registru de clienti) si il adaug in acesta, ori se deconecteaza , mesajul pe care l-am primit stiu ca este gol , si atunci ii inchid socketul.
Cazul in care se reconecteaza clientul-> ii este setat din nou socket-ul si ii se retrimit mesajele ale caror topicuri sunt cu subscriptie de tip sf.
Subscribe -> in aceasta situatie verific topicurile existente , daca gasesc topicul dorit pt abonare , atunci adaug in vectorul de clienti al topicului respectiv si clientul ce i-a dat subscribe ; daca nu gasesc topicul , atunci il construiesc si setez valorile pt primul client ce este abonat la acest topic.
Unsubscribe -> imi cauta topicul dorit pt a sterge clientul din vectorul de clienti specific topicului , odata gasit , sterge clientul si rearanjeaza vectorul.
	

	In client(subscriber) interpretez mesajele de la server in functie de cazurile specifice : float , string, int, short real si fac prelucrarile necesare pt afisare . Tot aici costruiesc mesajele pe care le trimit la server in functie de inputul pe care il dau de la tastatura : subscribe , unsubscribe.

	Am o problema si presupun ca este de la DIE . Nu stiu sigur si prefer sa o comunic . In momentul in care ii dau exit la subscriber imi da o eroare cu stack smashing . Nu imi afecteaza rezultatul programului. Daca inchid subscriber-ul prin combinatia de taste ctrl + c , atunci nu am aceasta eroare .

	
	
