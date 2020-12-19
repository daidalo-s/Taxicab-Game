# TaxiCab Game

Io speriamo che me la cavo

## Installazione
Dunque dunque, non so quale distro linux voi stiate utilizzando ma in ogni caso il comando dovrebbe essere abbastanza standard: ipotizzando stiate usando Ubuntu o una sua derivata il comando per installare git e tutto ciò che vi serve dovrebbe essere
```bash
sudo apt-get install git
```
Dopo il comando potete verificare l'installazione dando sempre sul terminale il comando git --version aspettandovi una cosa del genere (l'output è dal mio pc quindi il vostro sarà leggermente diverso).
```bash
lorenzo@Argos.local➜ ~ git --version
git version 2.24.3 (Apple Git-128)
```

## Inizializzazione
Dopo aver installato git è importante che inizializziate il nome e la mail per capire chi fa quale commit e non diventare stupidi a capire da dove arrivano certe modifiche. Lo potete fare con 
```bash
git config --global user.name "user_name"
git config --global user.email "email_id"
```
dove ovviamente al posto di user_name e email_id ci mettete il vostro username e la vostra mail.
Dopodiché dovete recarvi/crearvi una vostra cartella dove volete tenervi il progetto (mio consiglio createvi una cartella), all'inizio di questa pagina trovate un tasto verde con scritto code, ci cliccate sopra, copiate l'url che vi esce, prendete il terminale aperto sulla vostra cartella e date
```bash
git clone "url che avete copiato"
``` 
Se tutto ha funzionato dovreste trovarvi dentro la cartella Taxicab-Game.

Use the package manager [pip](https://pip.pypa.io/en/stable/) to install foobar.

```bash
pip install foobar
```

## Utilizzo
Come prima cosa dovete capire in qualche branch vi trovate. Potete farlo con
```bash
git branch
```
che vi mostra tutti i vari branch con un asterisco vicino a quello in cui siete attualmente (all'inizio sarà main). 
Quando volete lavorare per aggiungere una particolare funzione sarebbe meglio che anzichè lavorare nel main lavoraste in una vostra branch che vi create (l'idea è che il main è la "versione definitiva" che poi consegnamo, quello che fate dentro una vostra branch poi viene unito al contenuto del main in automatico e tutit felici e contenti): per farlo entrate nel terminale nella cartella Taxicab-game e date
```bash
git branch 'nomenuovabranch' ---> vi crea la nuova branch
git checkout 'nomenuovabranch'---> vi sposta dentro la nuova branch  
```
Dentro questa branch che vi siete creati (idealmente 'nomenuovabranch' dovrebbe essere un nome significativo che permetta di caprie a cosa state lavorando) potete sbizzarrirvi. Dopo che avete fatto un po' di modifiche provate a dare
```bash
git status 
```
e l'output dovrebbe essere qualcosa del tipo
```bash
lorenzo@Argos.local➜ VALUTATORE1 git:(branchsignificativa) ✗  git status                      
On branch branchsignificativa
Untracked files:
  (use "git add <file>..." to include in what will be committed)
	../PARSER2/.DS_Store
	../TRANSLATOR/Instruction$1.class
	../TRANSLATOR/Output.class
	brazellette.txt

nothing added to commit but untracked files present (use "git add" to track)
```
a questo punto date git add barzellette.txt per aggiungere il file a quelli tracciati da git e vi dirà
```bash
On branch branchsignificativa
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
	new file:   brazellette.txt

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	../PARSER2/.DS_Store
	../TRANSLATOR/Instruction$1.class
	../TRANSLATOR/Output.class 
```
a questo punto se volete caricare il nuovo file con le modifiche il comando è
```bash
git commit -m "messaggio in cui spiegate sinteticamente cosa avete fatto"
git push
```
e la magia è fatta.
## Ora la smetto ve lo giuro
Quando siete sicuri che quello che avete fatto possa andare a finire nel main (anche se poi magari ne parlemeno su come organizzarci per questo punto) dovete:
1. **Essere certi** di non avere modifiche in sospeso quando lanciate il comando git status
2. Tornare nella branch main con il comando
```bash
git checkout main     
```
3. Finalmente potete fare il merge con 
```bash
git merge --no-ff 'nomebranchsucuiavetelavoratochevoleteunirealmain'    
```
4. Eliminare la branch che non vi serve più con 
```bash
git branch -d 'nomebranchsucuiavetelavoratoidicuiavetefattoilmerge'  
```

## Problemi
Probabilmente ogni tanto potrebbero uscire dei problemi ma ne parleremo sicuramente