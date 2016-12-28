// indico di generare src assembly
// #pragma SRC

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>

#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h"
#include "target.h"
#include "fpga_nxp.h"

#include "spiext.h"
#include "spiprot.h"
#include "spimsg.h"
#include "spipass.h"
#include "spifmt.h"
#include "max519.h"
#include "lcd.h"
#include "minmax.h"
#include "gui.h"
#include "spiwin.h"
#include "az4186_dac.h"

extern int iReadCounter(void);
extern unsigned long lastRo;
extern unsigned char xdata ucBloccaVisualizzazioneMisuraStatica;
unsigned char xdata ucEliminaPrimoPezzo;
	extern void vVerifyInsertCodiceProdotto(xdata char *s);
extern void vLogStartProduzione(void);
extern void vLogEndProduzione(void);
//xdata float fCtrlOkResSta[64+1];

void vCalcAnticipoPretaglio(void);
float fCalcCoeffCorr(void);
TipoStructHandlePrg hprg;

xdata long tagliaMoncone_Imp;

extern data unsigned long roPrec;
/* Array dei valori letti dall' adc. */
xdat long int InputAnalog[NUM_INP_ANALOG];
xdat PROGRAMMA auxPrg;
extern xdata unsigned long ulContaMezzoGiro;

unsigned char SearchNextCommessa(unsigned char from);
unsigned char uc_use_hexadecimal_letters;


unsigned int ui_goto_temperature_compensation_params_window,ui_stop_forced;


void vCalcAnticipoPretaglio(void);
float fCalcCoeffCorr(void);




/* Routine che controlla
   - l' immissione di tasti;
   - start/stop della macchina;
   - misura statica e taratura dell' ohmetro della misura statica
*/

void inpToOut(void){
	xdat float f_aux;
	xdat unsigned int m,i;

   /* Gestione lampeggio lampada prima del termine del lotto. */
   /* Se devo cronometrare il tempo di accensione della lampada, lo faccio
      se NON vi sono allarmi. */
	if ((LampStatus&LampCount)&&(alr_status!=alr_status_alr)){
		if (LampStatus&LampOnOff){
			if (TempoLampada>SemiperiodoLampeggioLampada){
				/* Pongo a zero il tempo trascorso. */
				TempoLampada=0;
				/* Complemento lo stato del bit che controlla l' accensione della lampada. */
				outDigVariable ^= ODG_LAMPADA;
			}
		}
		/* Se la lampada � accesa ad indicare la fine del lotto, controllo se la devo spegnere. */
		else if (LampStatus&LampAccesaPeriodo){
			/* Se � passato un numero di secondi sufficiente a provocare lo spegnimento della lampada, lo faccio. */
			if (TempoLampada*PeriodoIntMs/1000.>=macParms.nSecLampSpegn){
				TempoLampada=0;
				/* Spengo la lampada. */
				outDigVariable &= ~ODG_LAMPADA;
				/* La lampada � spenta. */
				LampStatus=LampSpenta;
			}
		}
	}
	/* Aggiorno il numero di pezzi prodotti. */
	if (PrgRunning||RunningPrgEndOk)
		nvram_struct.commesse[spiralatrice.runningCommessa].ndo=actNumPezzi;
	if (RunningPrgEndOk){
		//TxCommessa(0);
		vLogEndProduzione();
	}
		/*	Se vi � un programma in esecuzione e il numero di pezzi � vicino a quello finale, faccio lampeggiare la lampada.
			La faccio lampeggiare anche se vi � un programma vuoto in fase di setup.
			Devo pertanto tener conto del tempo per sapere quando accenderla o spegnerla.
		*/
		if (    PrgRunning
			 &&( (   macParms.nPezziAvviso
			      &&(nvram_struct.commesse[spiralatrice.runningCommessa].n-nvram_struct.commesse[spiralatrice.actCommessa].ndo<=macParms.nPezziAvviso)
			      )
			    ||hprg.theRunning.empty
	            )
           ){
			LampStatus=LampOnOff;
		}
		/* Se:
			-	vi � un programma in esecuzione
			- non sto eseguendo delle prove su un programma a parametri non ancora memorizzati
			- non ho gi� eliminato il primo pezzo
			- � trascorso un tempo sufficiente a produrre un pezzetto di spirale che, essendo il primo, deve essere
				eliminato... 
			eseguo un taglio.
		*/
		if (  PrgRunning
			//&&!hprg.theRunning.empty
			&&!ucEliminaPrimoPezzo
			&&!spiralatrice.PrimoPezzoEliminato
			// 60mm * resistenza specifica ohm/m --> ottengo resistenza del moncone da tagliare
			&&(actLenImp>=tagliaMoncone_Imp)
			// (spiralatrice.TempoTaglioPezzo*PeriodoIntMs>=TaglioPezzoMs)
			){
			/* Metto lo stato dell' avvolgitore in attesa di pretaglio per permettere l' esecuzione del taglio. */
			avvStatus = AVV_WAIT_PRE;
			/* Indico che il primo pezzo deve essere eliminato. */
			ucEliminaPrimoPezzo=1;
			/* Indico che i pezzi prodotti vanno contati. */
			BitContaPezzi=1;
			/* Indico alla funzione timer di eseguire un taglio asincrono. */
			vDoTaglio();
		}

		/* Se:
			- viene impartito il comando di start e il motore non � gi� in moto, oppure il lotto corrente � finito correttamente
				e la macchina � programmata per lavorare in automatico
				ed il prossimo lotto puo' seguire il corrente senza fermo macchina
			- non sono nel menu ingressi/uscite,
			- non vi sono allarmi,
			- la resistenza del programma � valida;
			- la lunghezza del pezzo � valida; questi due controlli servono a
				stabilire se il prossimo programma � un programma valido o meno
				(potrebbe non essere mai stato editato);
	     allora parti con la produzione.
		*/
		/* Testo se:
			- se la commessa corrente � terminata correttamente;
			- la macchina � in automatico;
			- se vi � una commessa successiva: potrei incontrare una commessa a resistenza nulla (tappo della lista) od essere
				effettivamente all' ultima commessa della lista.
			- se � abilitata la comunicazione seriale, aspetto che il pezzo sia stato trasmesso prima di cancellare la commessa.
		*/
		if (   RunningPrgEndOk
			&& !(txModCont && txNewPz)
			&&(nvram_struct.commesse[spiralatrice.runningCommessa].automatico)
			/* Codice della prossima commessa da eseguire. */
			&&((spiralatrice.nextRunCommessa=SearchNextCommessa(spiralatrice.runningCommessa))!=spiralatrice.runningCommessa)
			&&(nvram_struct.commesse[spiralatrice.nextRunCommessa].res > 0.01)
		){
			/* Resetto l' indicatore di programma terminato correttamente. */
			RunningPrgEndOk=0;
			// lotto finito--> non segnalo errore in caso di tempo misura insufficiente
			// perch� potrei essere a cavallo di un periodo di misura...
			AlarmOnTimeout=0;
			LampStatus=LampAccesa;
			/* Accendo la lampada per indicare fine lotto. */
			outDigVariable |= ODG_LAMPADA;
			/* Controllo se il prossimo programma pu� essere lanciato senza fermare la macchina.
				Controllo che il numero di programma sia lo stesso.
			*/
			if (uiGetRunningPrg()!=nvram_struct.commesse[spiralatrice.nextRunCommessa].numprogramma){
				/* Se non posso proseguire, spengo il motore. */
				outDigVariable &= ~ODG_MOTORE;
				/* Resetto per sicurezza l' indicatore di misura in corso. */
				criticMis = 0;
				criticLav = 0;
				/* <--- Modifica del 07/04/1997 */
				TaglioAsincrono=0;
				/* Resetto le uscite di pretaglio e di taglio. */
				outDigVariable &= ~ODG_PRETAGLIO;
				outDigVariable &= ~ODG_TAGLIO;
				/* Non vi � programma in esecuzione. */
				PrgRunning=0;
				ucAlignJobsRunningAndSelected();
				/* Metto a zero le uscite di tutti i dac. */
				spiralatrice.StatusRampaDac=0;
				i=0;
				while (i<NumDacRampa*2){
					spiralatrice.DacOut[i]=0;
					spiralatrice.DacOutOld[i]=0;
					i++;
				}
				/* Azzero le uscite dei dac motore e ruote di contrasto. */
				SendByteMax519(addrDacMandrino,0,0);
				SendByteMax519(addrDacFrizione1,0,0);

				/* La macchina non pu� lavorare in automatico. */
				NextPrgAutomatic=0;
				/* Accendo la lampada per indicare fine ciclo. */
				// pero' lo devo fare solo se non � abilitata la lampada rossa di allarme,
				// come da specifiche per le modifiche gestione lampada rossa/gialla/verde
				if (!macParms.ucAbilitaLampadaAllarme){
					outDigVariable |= ODG_LAMPADA;
					/* La lampada � accesa fino al prossimo programma. */
					LampStatus=LampAccesa;
				}
				avvStatus = AVV_IDLE;
			}
			/* Se la macchina � disponibile a lavorare in automatico, devo passare
				al prossimo programma. */
			else{
				/* Indico finalmente che la macchina pu� lavorare in automatico. */
				NextPrgAutomatic=1;
				// INDICO ALLA FINESTRA SETUP PRODUZIONE CHE C'E' STATO UN CAMBIO DI COMMESSA
				ucSetWindowParam(enumWinId_setupProduzione,defWinParam_PROD_AutomaticProgramChange,1);

				/* Arresto momentaneamente l' esecuzione del programma. */
				PrgRunning=0;
				/* Indico che la lampada deve essere spenta dopo che � trascorso
					il tempo indicato come parametro macchina. */
				LampStatus=LampAccesaPeriodo;
				TempoLampada=0;
				/* Accendo la lampada per indicare fine lotto. */
				outDigVariable |= ODG_LAMPADA;
			}
			/* La commessa � terminata, la cancello. */
			DeleteCommessa(spiralatrice.runningCommessa);
#if 1
			// Michele versione 1.52: allineo anche running commessa per evitare problemi
			// di disallineamento tra lista nvram_struct.commesse e lista jobs
			//la purgeJoblist chiama la setactive product code che chiama la trasferisci da lista job a nvram_struct.commesse
			// che usa runningcommessa che deve essere allineato altrimenti viene saltata una commessa

			// passo alla prossima commessa
			i=SearchNextCommessa(spiralatrice.runningCommessa);
			/* Se non ne esiste una successiva, cerco la precedente. */
			if (i==spiralatrice.runningCommessa)
				i=SearchPrevCommessa(spiralatrice.runningCommessa);
			if (spiralatrice.actCommessa==spiralatrice.runningCommessa){
				spiralatrice.actCommessa=i;
				vInitActPrg();
			}
			// aggiorno l'indice running commessa perch� non posso lasciarlo puntare ad un elemento cancellato, 
			// a meno che la lista sia vuota, nel qual caso certamente non ci sar� un cambio commessa e non ci sono problemi
			spiralatrice.runningCommessa=i;
// prima della versione 1.52 il codice era cos�
#else

			if (actCommessa==runningCommessa){
				actCommessa=SearchNextCommessa(actCommessa);
				i=SearchNextCommessa(actCommessa);
				/* Se non ne esiste una successiva, cerco la precedente. */
				if (i==actCommessa)
					i=SearchPrevCommessa(actCommessa);
				actCommessa=i;
				vInitActPrg();
			}
#endif
			// aggiorno lista pJobsSelected_Jobs->..
			vTrasferisciDaCommesseAlistaLavori();
			// v1.29 pulisco anche tutti i codici che non hanno jobs!!!
			// in questa fase si crea il fastidio per cui se alcuni lavori sono stati inseriti vuoti nella joblist,
			// adesso vengono cancellati per cui c'� il pericolo che spariscono se erano stati appena inseriti come codici
			// ma non erano stati ancora appesi dei lotti di lavorazione

			// versione 1.52
			// quando un codice non ha pi� jobs in lista, NON viene automaticamente cancellato, ma rimane in lista
			// e deve essere cancellato manualmente
			// Pertanto commento la chiamata alla purge in modo che la cancellazione sia sempre manuale
			// ucPurgeJobList();

			vForceLcdRefresh();
		}
		/* Se il lotto � terminato, devo spegnere il motore anche se la macchina non pu� lavorare in automatico.
			Se la comunicazione seriale � abilitata, aspetto prima di
			cancellare la commessa.
		*/
		else if (   RunningPrgEndOk
				&& !(txModCont && txNewPz)
			){
			/* Resetto l' indicatore di programma terminato correttamente. */
			RunningPrgEndOk=0;
			// lotto finito--> non segnalo errore in caso di tempo misura insufficiente
			// perch� potrei essere a cavallo di un periodo di misura...
			AlarmOnTimeout=0;
			/* Indico di spegnere il motore. */
			i=0;
			while (i<NumDacRampa*2){
				spiralatrice.DacOut[i]=0;
				spiralatrice.DacOutOld[i]=0;
				i++;
			}
			outDigVariable &= ~ODG_MOTORE;
			/* Resetto per sicurezza l' indicatore di misura in corso. */
			criticMis = 0;
			criticLav = 0;
			/* <--- Modifica del 07/04/1997 */
			TaglioAsincrono=0;
			/* Resetto le uscite di pretaglio e di taglio. */
			outDigVariable &= ~ODG_PRETAGLIO;
			outDigVariable &= ~ODG_TAGLIO;
			/* Metto a zero le uscite di tutti i dac. */
			/* Indico che nessuna rampa deve essere aggiornata. */
			spiralatrice.StatusRampaDac=0;
			/* Azzero le uscite dei dac motore e ruote di contrasto. */
			SendByteMax519(addrDacMandrino,0,0);
			SendByteMax519(addrDacFrizione1,0,0);
			/* Indico che la macchina non pu� lavorare in automatico. */
			NextPrgAutomatic=0;

// versione 2.09
#if 1
			// Michele: accendo comunque la lampada gialla per indicare fine ciclo
			/* Accendo la lampada per indicare fine ciclo. */
			outDigVariable |= ODG_LAMPADA;
			/* La lampada � accesa fino al prossimo programma. */
			LampStatus=LampAccesa;
#else
			// accendo lampada per indicare fine ciclo
			// pero' lo devo fare solo se non � abilitata la lampada rossa di allarme,
			// come da specifiche per le modifiche gestione lampada rossa/gialla/verde
			if (!macParms.ucAbilitaLampadaAllarme){
				/* Accendo la lampada per indicare fine ciclo. */
				outDigVariable |= ODG_LAMPADA;
				/* La lampada � accesa fino al prossimo programma. */
				LampStatus=LampAccesa;
			}
			else{
				// spengo lampada gialla
				outDigVariable &= ~ODG_LAMPADA;
				/* La lampada gialla � spenta. */
				LampStatus=LampSpenta;
			}
#endif

			avvStatus = AVV_IDLE;
			PrgRunning=0;
			/* La commessa � terminata, la cancello. */
			DeleteCommessa(spiralatrice.runningCommessa);
#if 1
			// Michele versione 1.52: allineo anche running commessa per evitare problemi
			// di disallineamento tra lista nvram_struct.commesse e lista jobs
			//la purgeJoblist chiama la setactive product code che chiama la trasferisci da lista job a nvram_struct.commesse
			// che usa runningcommessa che deve essere allineato altrimenti viene saltata una commessa

			// passo alla prossima commessa
			i=SearchNextCommessa(spiralatrice.runningCommessa);
			/* Se non ne esiste una successiva, cerco la precedente. */
			if (i==spiralatrice.runningCommessa)
				i=SearchPrevCommessa(spiralatrice.runningCommessa);
			if (spiralatrice.actCommessa==spiralatrice.runningCommessa){
				spiralatrice.actCommessa=i;
				vInitActPrg();
			}
			// aggiorno l'indice running commessa perch� non posso lasciarlo puntare ad un elemento cancellato, 
			// a meno che la lista sia vuota, nel qual caso certamente non ci sar� un cambio commessa e non ci sono problemi
			spiralatrice.runningCommessa=i;
// prima della versione 1.52 il codice era cos�
#else
			if (actCommessa==runningCommessa){
				i=SearchNextCommessa(actCommessa);
				/* Se non ne esiste una successiva, cerco la precedente. */
				if (i==actCommessa)
					i=SearchPrevCommessa(actCommessa);
				actCommessa=i;
				vInitActPrg();
			}
#endif
			// aggiorno lista pJobsSelected_Jobs->..
			ucAlignJobsRunningAndSelected();

			vTrasferisciDaCommesseAlistaLavori();
			vForceLcdRefresh();
		}
		/* Dentro alla variabile NextPrgAutomatic adesso c' � il valore booleano della condizione:
			la macchina puo' proseguire in automatico col prossimo programma.
			La macchina puo' partire anche se � stato premuto il pulsante di start
			ed il motore non era gi� in moto, pertanto faccio l' or con la
			condizione di cui sopra.
		*/
		if (   (   NextPrgAutomatic
			    ||((   ucDigitalInput_PositiveEdge(enumInputEdge_start)) 
					&& !PrgRunning
					&& !ucIsDisabledStartLavorazione()
					&& !ucDigitalInput_AtHiLevel(enumInputEdge_stop)
					&&!sensoreTaglio.ucWaitInizializzazioneLama
					)
			   )
			&& (alr_status!=alr_status_alr)
			&& (nvram_struct.commesse[spiralatrice.actCommessa].res > 0.01)
			// deve esistere almeno una commessa per poterla eseguire!!!
			&& spiralatrice.firstComm
#ifndef defTest2fili
			&&!sLCD.ucSuspendDigitalIoUpdate
			//&& (  (actMenu!=MENU_UTILS)
			//	||((actSubmenu!=SBM_UT_INDIG)&&(actSubmenu!=SBM_UT_OUTDIG))
			//	)
#endif
			){
			v_reset_cutter_position();
			// azzero il numero di pezzi fatti, che mi serve per la stima del tempo residuo
			TPR.uiNumPezziFatti=0;
			vForceLcdRefresh();
			/* Azzero la variabile che indica se vi deve essere una partenza a basso numero di rpm.
				Quand' e' che devo partire con rpm bassi?
					1) codice prodotto non memorizzato: il max valore in rpm
						del mandrino deve essere quello massimo effettivo
					2) nuova commessa (codice prodotto gia' memorizzato):
						il max valore in rpm del mandrino e' quello memorizzato
					3) vi e' stato un allarme filo aggrovigliato o interruzione filo
			*/
			/* Se si era verificato un allarme per cui devo partire piano... */
			if (spiralatrice.ucAlarmSlowRpm){
				/* Parto piano a causa di un allarme. */
				spiralatrice.slowRpmMan=slowRpmManAll;
				/* Azzero il flag. */
				spiralatrice.ucAlarmSlowRpm=0;
			}
			/* Altrimenti, finora potrei pure partire veloce. */
			else{
				spiralatrice.slowRpmMan=noSlowRpmMan;
			}
			/* Se sono in automatico, conosco il numero della prossima commessa. */
			if (NextPrgAutomatic){
				spiralatrice.slowRpmMan=noSlowRpmMan;
				//anche in automatico, partenza lenta. 
				// no! 1.42 in automatico proseguo con velocit� impostata!!!!
				//spiralatrice.slowRpmMan=slowRpmManCom;
				spiralatrice.runningCommessa=spiralatrice.nextRunCommessa;
				spiralatrice.actCommessa=spiralatrice.runningCommessa;
				vInitActPrg();
				if (actMenu==MENU_SETUP)
					menuOperation(MNU_OP_PRINT);
			}
			else{
				// 1.28
				// all'avvio del programm aalzo il coltello!!!
				vAttivaOperazioneLama(enumCiclicaLama_sali);

				// se � uno start da fermo, azzero le variabili encoder
				#if 1==1
					// Azzero tutte le grandezze che esprimono il numero
					// di conteggi sulla resistenza attuale.

                  T0TCR = 0; // disable timer
                    oldEncLav = oldEncMis = newEnc = iReadCounter();
                  T0TCR = 1; // enable timer

				#endif

				/* Inizializzo il valore della resistenza ideale che verra' usata
					per eventuali tarature alle estremita' della spirale.
				*/
				fResTaraturaFine=nvram_struct.commesse[spiralatrice.runningCommessa].res;
				/* Delta R di resistenza nullo. */
				fResDeltaR=0;
				/* In non automatico, sempre partenza lenta. */
				if (spiralatrice.slowRpmMan==noSlowRpmMan)
           			spiralatrice.slowRpmMan=slowRpmManCom;
				spiralatrice.runningCommessa=spiralatrice.actCommessa;
				/* Se non sono in automatico, il primo pezzo va eliminato... */
				spiralatrice.TempoTaglioPezzo=0;
				// se uso il distanziatore, il punto di inizio di produzione della resistenza � fissato 
				// sul distanziatore, pertanto non � necessario eliminare il moncone,
				// viene fatto in automatico
				if (   macroIsCodiceProdotto_Distanziatore(&hprg.theRunning)
					&&(macParms.uc_distanziatore_type!=enum_distanziatore_type_coltello)
					){
					spiralatrice.PrimoPezzoEliminato=1;
				}
				else{
					spiralatrice.PrimoPezzoEliminato=0;
				}
				ucEliminaPrimoPezzo=0;
				/* Azzero il numero di pezzi di scarto. */
				spiralatrice.NumPezziScarto=0;
				/* Inizializzo il valore della velocita' in metri al minuto. */
				velMMin=0;
				/* Annullo anche lo spazio ed il tempo che uso per il calcolo della velocita'. */
				spiralatrice.spaziorv=0;
				spiralatrice.temporv=0;
				oldnImpulsi=newnImpulsi=nImpulsi=0;
       			spiralatrice.slowRpmMan=slowRpmManCom;
			}
			/* Comincia una nuova commessa, resetto il valore della resistenza statica usato per calcolare il deltar.*/
			spiralatrice.oldresStaMediata=1e30;
			/* Dato che la commessa sta iniziando, salvo i dati di inizio commessa. */
			DatiInizioCommessa();
			// devo rinfrescare lcd...
			vForceLcdRefresh();
			/* Indico che si deve premere F5 nel menu pars dinamici per abilitare
				la taratura tramite ohmetro statico. */
			spiralatrice.OkAbilitaTaraturaStat=0;
			spiralatrice.OkAbilitaTaraFineStat=0;
			/* Mi procuro un puntatore al record della commessa in esecuzione. */
			spiralatrice.pCommRun=&nvram_struct.commesse[spiralatrice.runningCommessa];
			uiSetRunningPrg(spiralatrice.pCommRun->numprogramma);
			/* Mi procuro un puntatore al record del programma in esecuzione. */
			spiralatrice.pPrgRun=&hprg.theRunning;
			/* Se il programma � vuoto, posso da subito modificare i potenziometri,
				altrimenti devo premere un tasto funzione per poterlo fare. */
			/* Modifica Gennaio 1999: i potenziometri sono abilitati anche se devo partire con velocita' bassa
			*/
			if (spiralatrice.pPrgRun->empty){
				/* Modifica Gennaio 1999: se programma vuoto, devo partire con velocita' bassa e
					max rpm mandrino prg= max rpm effettivi mandrino
				*/
				spiralatrice.slowRpmMan=slowRpmManPrg;
			}
			/* Se parto a velocita' normale, i potenziometri devono essere abilitati. */
			if (spiralatrice.slowRpmMan==noSlowRpmMan){
				spiralatrice.AbilitaModificaPotenziometri=1;
			}
			/* Se parto a velocita' bassa, i potenziometri sono attivi. */
			else{
				spiralatrice.AbilitaModificaPotenziometri=0;
				// imposto numero di giri bassi nella commessa
				spiralatrice.pCommRun->rpm=RPM_MAND_PRG_EMPTY;
			}
			/* Non clippare la velocita' al massimo, per il momento. */
			spiralatrice.ucClipVelocity=0;
			/* Calcolo la resistenza specifica della spira dal parallelo
				delle res specifiche delle singole spire.
			*/
			spiralatrice.pCommRun->resspec_spir=f_calc_res_spec_filo(spiralatrice.pPrgRun);	
			/* Calcolo la resistenza di uno spezzone; dai millimetri del diametro
				tolgo 2 per tenere conto della resistenza dei contatti su cui si
				appoggia lo spezzone.
			*/
			f_aux=f_calc_res_spezzone(spiralatrice.pCommRun->resspec_spir);
/* Per essere sicuri di non dover effettuare un cambio portata,
				aumento il valore della resistenza dello spezzone di una certa
				percentuale. Dato che ho a disposizione una tabella dei fornitori
				che mi consente di determinare la tolleranza sulla misura della
				resistenza, uso questo valore controllando che non sia troppo
				basso (capita solo se l' entrata della tabella non � stata
				inizializzata).
			*/
			if (nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore]<MIN_VAR_RES)
				nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore]=MIN_VAR_RES;
			/* Se tolleranza troppo bassa, uso tolleranza di default,
				solo per calcolare la portata adeguata. */
			if (nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore]<MIN_VAR_RES_CALC_PORTATA)
				f_aux*=1.+MIN_VAR_RES_CALC_PORTATA/100.;
			else
				f_aux*=1.+nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore]/100.;
			/* Adesso conosco quale portata utilizzare per effettuare le misure. */
			spiralatrice.pCommRun->defPortata=us_calc_portata_dinamica_per_resistenza(f_aux);

		/* Parto col numero di pezzi della commessa che sto eseguendo. */
		actNumPezzi=nvram_struct.commesse[spiralatrice.runningCommessa].ndo;
		/* Salvo il numero di pezzi con cui parto. */
		spiralatrice.oldndo=actNumPezzi;
		/* Tutti i seguenti calcoli vanno fatti se il programma non � in automatico, altrimenti � inutile ripeterli ad ogni
			cambio commessa. */
		if (!NextPrgAutomatic){
			/* Mostro la finestra coi parametri. */
			if ((actMenu!=MENU_EDIT_COMM)||(actSubmenu!=SBM_EC_PARAM_PRG)){
				actMenuBck=actMenu;
				actSubmenuBck=actSubmenu;
				actMenu=MENU_EDIT_COMM;
				actSubmenu=SBM_EC_PARAM_PRG;
			}
			hDist.uiEnabled_interrupt=0;
			// set alfa factor to use if the program uses temperature coefficient!
			if (macroIsCodiceProdotto_TemperatureCompensated(spiralatrice.pPrgRun)){
				// set the temperature coefficient
				v_set_alfa_factor(spiralatrice.pPrgRun->ui_temperature_coefficient_per_million*0.000001);
				if (!ui_temperature_compensation_valid()){
					ui_goto_temperature_compensation_params_window=0;
					ui_stop_forced=1;
					ucCallWindow(enumWinId_Temperature_Compensation_Params);
				}
				else{
					ui_goto_temperature_compensation_params_window=0;
				}

			}
			// if no temperature adjustment, set alfa factor to zero
			else{
				v_set_alfa_factor(0.000);
			}
			if (macroIsCodiceProdotto_Distanziatore(spiralatrice.pPrgRun)){

				// Azzero il numero totale di distanziazioni da effettuare 
				hDist.runPrgNumDistanzia=0;
				// Finche' non supero il numero massimo e la percentuale finale indicata � diversa da zero, proseguo col conteggio. 
				while( (hDist.runPrgNumDistanzia<defMaxNumOfPercDistanzia)
						&&(spiralatrice.pPrgRun->usPercDistanzia_Ends[hDist.runPrgNumDistanzia])
					){
					hDist.runPrgNumDistanzia++;
				}
				// Azzero il numero di distanziazioni correntemente effettuate; in realta' lo imposto al max in modo che la
				// prossima resistenza parta dalla prima distanziazione
				hDist.actNumDistanzia=hDist.runPrgNumDistanzia;
				hDist.actResDistValue=0;
				// la resistenza precedentemente prodotta aveva il valore finale 
				hDist.precResDistValue=TARGET_URP;
				// vale 1 se e' in corso una distanziazione 
				hDist.ucDistanziaInCorso=0;
				// azzero il flag che da' il via alle distanziazioni, viene messo ad 1 appena la resistenza calcolata sul
				// distanziatore supera il valore nominale, vale a dire appena parte la produzione 
				hDist.ucStartDistanzia=0;
				// metto il distanziatore nello stato idle
				hDist.stato=enum_coltello_distanzia_stato_idle;
				hDist.deltaDistValue=0;
				// s euso il coltelllo, il parametro offset distanziatore indica il numero di spire
				// di cui si comprime la spirale quando abbasso il distanziatore prima che le spire inizino a distanziarsi
				// calcolo resistenza in urp/impulsi di N spire
				// calcolo resistenza di N spire, divido per resistenza totale e moltiplico per target_urp
				hDist.res_spire_compressione_urp_imp=0;
				if (macParms.uc_distanziatore_type==enum_distanziatore_type_coltello){
					if ((macParms.modo == MODE_LENR)&&(spiralatrice.pCommRun->ucWorkingMode_0L_1LR))
						hDist.res_spire_compressione_urp_imp=spiralatrice.pPrgRun->f_coltello_offset_n_spire*spiralatrice.pCommRun->resspec_spir*0.001*PI_GRECO*(spiralatrice.pPrgRun->diametro_mandrino+spiralatrice.pPrgRun->diametro_filo)/spiralatrice.pCommRun->res*TARGET_URP;
					else
						hDist.res_spire_compressione_urp_imp=spiralatrice.pPrgRun->f_coltello_offset_n_spire*PI_GRECO*(spiralatrice.pPrgRun->diametro_mandrino+spiralatrice.pPrgRun->diametro_filo)/impToMm;
				}
			}
		
			/* Modifica Gennaio 1999: numero di spire di pretaglio puo' essere
				aggiustato anche con macchina in funzione.
			*/
			vCalcAnticipoPretaglio();
			/* Programmazione dei Dac. */
			/* Il comando da impartire al Dac di controllo del mandrino
				 deve indicare la velocit� angolare del mandrino nel range
				00..FF  <-->  min_rpm..max_rpm
				La velocit� di produzione � data in metri/minuto, conoscendo
				il diametro del mandrino ricavo la velocit� angolare in
				giri/minuto, e da questa ottengo il codice per il DAC.
				vel_metri_al_minuto/raggio_mandrino_in_metri=vel_rad_al_minuto
				vel_rad_al_minuto/2*pi_greco=vel_giri_minuto
				oppure, pi� rapidamente:
				vel_metri_al_minuto/(pi_greco*diametro_mandrino_in_metri)=rpm
				(rpm_mandrino*FS_DAC)/max_rpm_mandrino=codice da inviare al dac.
				In tutti i calcoli ci si deve ricordare di introdurre gli
				aggiustamenti effettuati tramite potenziometro.
			*/
			/* Azzero il valore precedente delle uscite verso i dac. */
			i=0;
			while (i<NumDacRampa*2){
				spiralatrice.DacOut[i]=0;
				spiralatrice.DacOutOld[i]=0;
				i++;
			}
			/* Indico di partire dal primo gradino della rampa. */
			spiralatrice.GradinoRampaDAC=0;

			/* Porto in spiralatrice.DacOutValue[0] la velocit� angolare del motore. */
			/* Modifica Gennaio 1999: e' possibile che io debba partire lento. */
			if (spiralatrice.slowRpmMan==noSlowRpmMan){
				spiralatrice.DacOutValue[0]=spiralatrice.pPrgRun->rpm_mandrino;
			}
			else{
				spiralatrice.DacOutValue[0]=RPM_MAND_PRG_EMPTY;
			}

			/* Chiamo la procedura di settaggio dei potenziometri. */
			SetPotenziometri();

			spiralatrice.DacOutMaxValue[0]=macParms.rpm_mandrino;
			/* La coppia del mandrino non viene gestita. */
			spiralatrice.DacOutValue[1]=1;
			spiralatrice.DacOutMaxValue[1]=1;
			SetDacValue(addrDacMandrino);

			/* Ricalcolo per sicurezza le velocita' delle ruote. */
			AdjustVelPeriferica(0);
			i=0;
			while (i<NUM_FRIZIONI){
				/* Aggiustamento della velocit� in rpm della ruota i. */
				spiralatrice.DacOutValue[0]=spiralatrice.pPrgRun->vruote_prg[i];
				/* Il valore della velocit� lo ottengo da quello nominale
				   (che dipende dagli rpm del mandrino) modificandolo
				   secondo quanto stabilito dal potenziometro. */
				spiralatrice.DacOutValue[0]=(((long)spiralatrice.DacOutValue[0])*2L*spiralatrice.pPrgRun->vel_frizioni[i])/FS_ADC;
				/* Valore massimo della velocit� della routa. */
				spiralatrice.DacOutMaxValue[0]=macParms.rpm_frizioni[i];
				/* La coppia non viene gestita, � indifferente. */
				spiralatrice.DacOutValue[1]=1;
				spiralatrice.DacOutMaxValue[1]=1;
				/* Chiamo la routine con l' indirizzo corretto. */
				SetDacValue(addrDacFrizione1+i);
				i++;
			}

			vAggiornaPosizioneColtello();

			/* Piloto il coltello. */
			spiralatrice.SommaRaggiMenoInterasse=(spiralatrice.pPrgRun->diametro_filo+spiralatrice.pPrgRun->diametro_mandrino+macParms.diam_coltello)/2.-macParms.iasse[0];
			if (spiralatrice.SommaRaggiMenoInterasse>spiralatrice.pPrgRun->pos_pretaglio)
				spiralatrice.actPosDacColtello=spiralatrice.SommaRaggiMenoInterasse-spiralatrice.pPrgRun->pos_pretaglio;
			else
				spiralatrice.actPosDacColtello=PercPosInizialeColtelloPre*spiralatrice.CorsaColtello;
			/* Invio il comando di posizionamento del coltello. */
			spiralatrice.DacOut[addrDacColtello*2]=PercPosInizialeColtello*FS_DAC_MAX519;
			spiralatrice.DacOut[addrDacColtello*2+1]=spiralatrice.actPosDacColtello*FS_DAC_MAX519/spiralatrice.CorsaColtello;
			SendByteMax519(addrDacColtello,spiralatrice.DacOut[addrDacColtello*2],spiralatrice.DacOut[addrDacColtello*2+1]);
		}
		/* Disabilito le interruzioni dal timer 0 mentre azzero le grandezze di interesse. */
//		ET0=0;
		actSpezInPez = 0;
		actSpezAvv   = 0;
		actLenImp    = 0;
		resLastSpezUp= 0;
		actRes       = 0;
		impFromMis   = 0;
		ulContaMezzoGiro=0;
//		ET0=1;
		menuOperation(MNU_OP_PRINT);
		// calcolo  la lunghezza del filo tra distanziatore e coltello */
		hDist.fLenFiloDistColtMm=(macParms.fOffsetDistanziatore_ColtelloMm/spiralatrice.pPrgRun->diametro_filo)*PI_GRECO*(spiralatrice.pPrgRun->diametro_mandrino+spiralatrice.pPrgRun->diametro_filo);

		/* Se la macchina � dotata di dispositivo per la misura dinamica ed � stata selezionata la modalit� L+R, avviso la routine
			di interruzione del timer 0 che � attiva la modalit�
			L+R tramite la variabile withMis. 
		*/

		// lavoro in modo L+R se modo L+R abilitato da setup e se la lista lavori corrente
		// ha modo L+R abilitato
		if ((macParms.modo == MODE_LENR)&&(spiralatrice.pCommRun->ucWorkingMode_0L_1LR)) {
			withMis = 1;
			/* Salvo nella variabile targetRes il valore di resistenza da produrre. */
			targetRes = spiralatrice.pCommRun->res;

			/* uOhm per URP */
			/* Calcolo il fattore di conversione dalla resistenza in urp
				alla resistenza in ohm. L' inverso di questo fattore permette
				di convertire la resistenza in urp.
				RES_PER_URP converte urp in microohm, pertanto se x contiene
				un valore di resistenza in urp,
				x*RES_PER_URP*targetRes=x*1e6/TARGET_URP*targetRes
				x/TARGET_URP � la frazione di resistenza realizzata
				x/TARGET_URP*targetRes � il valore in ohm della resistenza
				realizzata, moltiplicando per 1e6 si ottiene il valore in
				microohm.
			*/
			urpToRes = targetRes * RES_PER_URP + ROUND;
			/* Indico che le informazioni sul filo devono essere ricreate. */
			spezFilo.flags = 0;
			rdQueSpez      = 0;
			wrQueSpez      = 0;
		
			/* La variabile mmRuotaAvv vale MM_RUOTA1_AVV, ed � forse
				la distanza dalla prima torretta all' avvolgitore. */
			/* Le variabili nSpezDelay e chunkAvv sono intere. */
			/* delay= numero di spezzoni dalla torretta all' avvolgitore.
				(compreso quello sulla torretta)
			*/
			nSpezDelay = mmRuotaAvv / impToMm / macParms.lenSemigiro + 1;
			/* Blocco avv
				numero di impulsi prima che lo spezzone sulla torretta
				arrivi all' avvolgitore
			*/
			chunkAvv = nSpezDelay * macParms.lenSemigiro - mmRuotaAvv / impToMm;
		  /* Calcolo il numero di impulsi che devo ricevere per poter dire
			 di aver completato il pezzo piu' una tolleranza del 20%. */
			targetLenImp = (long)((spiralatrice.pCommRun->res/fCalcCoeffCorr())/spiralatrice.pCommRun->resspec_spir *1000./ impToMm* 1.2  + ROUND);

			/* Il valore nominale della resistenza specifica in
			 urp_per_impulso lo trovo come:
				TARGET_URP/targetRes
				urp/res_pezzo--> urp_per_ohm                             |-->urp_per_impulso
				resspec_spir   ImpToMm       /1000
				*res_spec_spira*mm_per_impulso*m_per_mm->ohm_per_impulso |
				MAGN_LEN
				*fatt_scala_len*allungamento+arrotondamento
			*/
			nomRo = actRo =  TARGET_URP * MAGN_LEN * impToMm / 1000. * spiralatrice.pCommRun->resspec_spir /targetRes + ROUND;
			roPrec=nomRo;
			{
				extern unsigned char uc_reinit_ro_with_temperature_compensation;
				// stated that ro changes with temp, if using compensation I should reinit ro with the first value measured  instead using the nominal one
				// else with big changes in temperature the tolerance control fails beacuse the temperature compensation goes to an incorrect ro!
				uc_reinit_ro_with_temperature_compensation=1;
			}
			lastRo=nomRo;
			ulMaxTrueRo=(nomRo*(100+(macParms.ucPercMaxErrRo+nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore])))/100;
			ulMinTrueRo=(nomRo*(100-(macParms.ucPercMaxErrRo+nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore])))/100;
			ucMaxBadSpezzConsec=macParms.ucMaxBadSpezzConsec;
			ucActBadSpezzConsec=0;
				hDist.actRoDist=actRo*fCalcCoeffCorr();

			/* Calcolo il fattore di correzione della resistenza specifica
				 dello spezzone. */
			actCorrOhmica=fCalcCoeffCorr()*1024.0;
			minRo = nomRo * (1. - ((float)nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore]) / 100.);
			maxRo = nomRo * (1. + ((float)nvram_struct.TabFornitori[spiralatrice.pPrgRun->fornitore]) / 100.);
			hDist.anticipoDistanz=hDist.fLenFiloDistColtMm/spiralatrice.pPrgRun->num_fili*spiralatrice.pCommRun->resspec_spir/1000.0/targetRes*TARGET_URP+0.5;
			// Imposto la resistenza attuale in modo che il primo taglio
			// venga effettuato in maniera sincrona con il distanziatore.
			// Quando verr� prodotta una resistenza di valore anticipoDistanz
			// verr� eseguito un taglio.
			if (   macroIsCodiceProdotto_Distanziatore(spiralatrice.pPrgRun)
				&&(macParms.uc_distanziatore_type!=enum_distanziatore_type_coltello)
				){
				//ET0=0;
				resLastSpezUp=TARGET_URP-(((int)(hDist.anticipoDistanz*fCalcCoeffCorr()))%TARGET_URP);
				actRes=resLastSpezUp;
				//ET0=1;
			}
			else{
				actRes = 0;
			}
			/* Indico che mancano ancora molti impulsi alla fine della spira. */
			preLenImp=0xFFFFFFL;
			/* Indico di partire da misura off line. */
			misStatus=MIS_OFF_LINE;
		}
		/* Se sono in modalit� L, ... */
		else {
			targetRes = spiralatrice.pCommRun->res;
			actCorrOhmica=fCalcCoeffCorr()*1000;

			withMis = 0;
			/* Calcolo il numero di impulsi che devo ricevere per poter dire
				di aver completato il pezzo. */
			f_aux=(spiralatrice.pCommRun->res/fCalcCoeffCorr());
			f_aux=f_aux/spiralatrice.pCommRun->resspec_spir;
			f_aux=(f_aux *1000./ impToMm  + ROUND);
			targetLenImp = (long)f_aux;
			hDist.actResDistValue=hDist.precResDistValue=targetLenImp;
			hDist.anticipoDistanz=hDist.fLenFiloDistColtMm/impToMm;
			// Imposto la resistenza attuale in modo che il primo taglio
			// venga effettuato in maniera sincrona con il distanziatore.
			// Quando verr� prodotta una resistenza di valore anticipoDistanz
			// verr� eseguito un taglio.
			if (   macroIsCodiceProdotto_Distanziatore(spiralatrice.pPrgRun)
				&&(macParms.uc_distanziatore_type!=enum_distanziatore_type_coltello)

				){
				//ET0=0;
				actLenImp=targetLenImp-((int)(hDist.anticipoDistanz*fCalcCoeffCorr()))%targetLenImp;
				//ET0=1;
			}

				
			if (anticipoPretaglio>targetLenImp)
				anticipoPretaglio=(unsigned short)(9*targetLenImp/10);
			preLenImp = targetLenImp - anticipoPretaglio; /* per far partire il comando di pretaglio */
		}

		/* Setto lo stato del mandrino in: attesa del comando di pretaglio. */
		avvStatus = AVV_WAIT_PRE;
		/* Se parto con la macchina ferma, devo avere un valore
			base per il conteggio del tempo e del numero degli
			impulsi. */
		if (!NextPrgAutomatic){
			updVel=0;
			oldgapT=gapT;
			oldnImpulsi=nImpulsi;
			/* Metto a verde il semaforo che indica la possibilita' di aggiornare
				il tempo trascorso. */
			updVel=1;
			/* Pongo a zero spazio e tempo. */
			tempo=0;
			spazio=0;
			/* Spengo la lampada se non sono in automatico. */
			LampStatus=LampSpenta;
			/* Spengo la lampada. */
			outDigVariable &= ~ODG_LAMPADA;
			/* Pongo a zero il contatore del tempo trascorso. */
			TempoLampada=0;
				// col distanziatore non devo contare il primo pezzo fatto...
				if (   macroIsCodiceProdotto_Distanziatore(spiralatrice.pPrgRun)
				    &&(macParms.uc_distanziatore_type!=enum_distanziatore_type_coltello)

					){
					BitContaPezzi=0;
				}
				else{
					// Indico che i pezzi prodotti vanno contati: se l' operatore
					//non preme F1, almeno sa quanti pezzi ha fatto. 
					BitContaPezzi=1;
				}
		}
		/* Se sto lavorando in automatico con la misura ohmica e
			passo alla commessa successiva con la misura della torretta
			attiva, puo' darsi che la macchina non ce la faccia
			ad effettuare la misura nel tempo rimanente; setto pertanto
			un bit che indica di non tenere conto dell' eventuale timeout
			nella prima misura.
		*/
		else{
			if (withMis&&(!(InpDig & IDG_MIS_VALID1)))
				/* Indico di non segnalare allarme in caso di timeout. */
				AlarmOnTimeout=0;
		}
		/* Resetto l' indicatore di macchina in passaggio automatico
			da un programma al successivo. */
		NextPrgAutomatic=0;

		/* Vi � un programma in esecuzione. */
		PrgRunning=1;

	    /* Accendo il motore. Lo accendo anche se sono
			in automatico, per sicurezza. */
		outDigVariable |= ODG_MOTORE;

		// Michele: abilito la gestione distanziatore solo se il codice prodotto lo prevede...
		if (macroIsCodiceProdotto_Distanziatore(spiralatrice.pPrgRun)){
			hDist.uiEnabled_interrupt=1;
		}
		// chiamo la procedura di log dello start produzione
		vLogStartProduzione();
	}

	/* Altrimenti, se:
		- sono in stop oppure vi � un allarme;
		- ed il motore � in moto
      ferma il motore ed entra in IDLE state.
	*/
	else if (    ucDigitalInput_PositiveEdge(enumInputEdge_stop)  
		      || (alr_status==alr_status_alr)
			  || (ui_stop_forced)
			){
		ui_stop_forced=0;
		// se sono nel menu boccola, non devo muovere la lama
		if (nvram_struct.ucImAmImMenuBoccola_0x37!=0x37){	
			v_reset_cutter_position();
		}
		if (PrgRunning)
			vLogStopProduzione();
			//TxCommessa((ucDigitalInput_PositiveEdge(enumInputEdge_stop))?1:2+(((unsigned int)alarms)<<8));
		/* Commuto sulla pagina degli allarmi, se non ci sono gi�. */
		if (  (alarms!=spiralatrice.oldalarms)
			&&(alr_status==alr_status_alr)
			&&((actMenu!=MENU_UTILS)||(actSubmenu!=SBM_UT_ALR))
			){
			vLogAlarm();
			menuOperation(MNU_OP_PRINT);
			spiralatrice.oldalarms=alarms;
		}
		/* Non vi � un programma in esecuzione. */
		PrgRunning=0;
		// trasferisco i dati da nvram_struct.commesse a lista lavori
		vTrasferisciDaCommesseAlistaLavori();
		ucAlignJobsRunningAndSelected();
		withMis=0;
		/* Se allarmi, accendo lampada. */
		if (alr_status==alr_status_alr){
			// per segnalare allarme,
			// se non esiste la lampada rossa di gestione allarme, uso
			// la lampada gialla
			if (!macParms.ucAbilitaLampadaAllarme){
				outDigVariable |= ODG_LAMPADA;
				/* La lampada � accesa. */
				LampStatus=LampAccesa;
			}
			else{
				// accendo lampada rossa
				outDigVariable |=  ODG_LAMPADA_RED;
				// spengo lampada gialla
				outDigVariable &= ~ODG_LAMPADA;
				/* La lampada � spenta. */
				LampStatus=LampSpenta;
			}
		}
		else{
			/* Spengo la lampada gialla. */
			outDigVariable &= ~ODG_LAMPADA;
			/* La lampada � spenta. */
			LampStatus=LampSpenta;
			// spengo la lampada rossa
			outDigVariable &= ~ODG_LAMPADA_RED;
		}
		/* Indico che nessuna rampa deve essere aggiornata. */
		spiralatrice.StatusRampaDac=0;
		spiralatrice.GradinoRampaDAC=NumGradiniRampaDAC;
		/* Se il motore � acceso, lo spengo azzerando i DAC del mandrino
			e delle frizioni. */
		i=0;
		while (i<NumDacRampa*2){
			spiralatrice.DacOut[i]=0;
			spiralatrice.DacOutOld[i]=0;
			i++;
		}
		/* Azzero le uscite dei dac motore e ruote di contrasto. */
		SendByteMax519(addrDacMandrino,0,0);
		SendByteMax519(addrDacFrizione1,0,0);
		/* Spengo il motore. */
		outDigVariable &= ~ODG_MOTORE;

	    /* Resetto per sicurezza l' indicatore di misura in corso. */
		criticMis = 0;
		criticLav = 0;
		/* <--- Modifica del 07/04/1997 */
		TaglioAsincrono=0;
		NextPrgAutomatic=0;
		/* Resetto le uscite di pretaglio e di taglio. */
		outDigVariable &= ~ODG_PRETAGLIO;
		outDigVariable &= ~ODG_TAGLIO;
		// se interviene stop o allarme, azzero l'uscita distanziatore
		outDigVariable &= ~ODG_DISTANZIA;
		// fermo la gestione in interrupt del distanziatore...
		hDist.uiEnabled_interrupt=0;
		// Michele 5 aprile 2012--> forzo sempre la lama a salire, dopo uno stop!
		// se sono nel menu boccola, non devo muovere la lama
		if (nvram_struct.ucImAmImMenuBoccola_0x37!=0x37)	
		{
				// ovviamente comando lama alta solo se non � gi� alta...
				unsigned int ui_lama_a_riposo;
				ui_lama_a_riposo=
									 (    (sensoreTaglio.uiLetturaSensoreTaglio>sensoreTaglio.uiPosRiposo)
									  &&((sensoreTaglio.uiLetturaSensoreTaglio-sensoreTaglio.uiPosRiposo)<100)
									 )
								   ||
									(    (sensoreTaglio.uiLetturaSensoreTaglio<sensoreTaglio.uiPosRiposo)
									  &&((sensoreTaglio.uiPosRiposo-sensoreTaglio.uiLetturaSensoreTaglio)<100)
									) ;		
				if (!ui_lama_a_riposo){
					vAttivaOperazioneLama(enumCiclicaLama_sali);
				}
		}
		hDist.ucStartDistanzia=0;
		if (!TaglioAsincrono)
			avvStatus = AVV_IDLE;
	}

// se premo misura statica e sono nel menu taratura dinamica, devo visualizzare cosa misura ohmetro dinamico
// con portata correntemente usata 
//	     spiralatrice.actCanPortata=(actSubmenu<SBM_TARAT_MIS_DYN1K)?N_CANALE_OHMETRO_STA:N_CANALE_OHMETRO_DYN;
	if (     (myInpDig & IDG_MSTA)
		   &&(!misStaSleep||(misStaStatus ==MIS_OFF_LINE))
		   &&(ucIamInTaraturaDinamica()||ucIamInLogProduzione())
		   &&!PrgRunning
	      ){
		/* Lo stato della misura statica e' commutazione multiplexer */
		misStaStatus=MIS_COMM_MUX;
		sLCD.ucAttivataMisuraDinamica=1;
		if (!spiralatrice.StartPortata){
			spiralatrice.StartPortata=1;
			if (ucIamInLogProduzione()){
				// calcolo la portata necessaria per realizzare la resistenza dello spezzone data la resistenza specifica del filo
				// usato dal programma attuale
				actStaPortata=us_calc_portata_dinamica_per_resistenza(f_calc_res_spezzone(f_calc_res_spec_filo(&hprg.theRunning)));
			}
			else if (actPortata>NUM_PORTATE_DINAMICHE-1)
				actStaPortata=0;
			else
				actStaPortata=NUM_PORTATE_DINAMICHE-1-actPortata;
			vUpdateDacPortValue((enum_select_dac_portata)actStaPortata);
			misStaSleep = SLEEP_CHANGE*2;
			goto NonFareMisSta;
		}
		/* Salvo il contatore attuale che indica il numero di cicli di interrupt. */
		spiralatrice.misUpdate =iArrayConvAD7327[N_CANALE_OHMETRO_DYN];

		/* Controllo per sicurezza che non sia subentrata un' interruzione... */
		if (!PrgRunning){
			/* Altrimenti ho una misura che indica morsetti aperti oppure una misura valida. */
			/* Mi procuro un puntatore al membro dell' array delle
				tarature che viene chiamato in causa. */
			pTar=&macParms.tarParms[N_CANALE_OHMETRO_DYN][actStaPortata];
			/*misStaStatus=MIS_OFF_LINE;*/
			if (spiralatrice.misUpdate >= MAX_FS_ADC_STATICA){
				spiralatrice.resStaMediata = 2e30;
				spiralatrice.resStaMediataAux=0.0;
				if (++spiralatrice.nSampleResSta>=N_SAMPLE_RES_STA){
					spiralatrice.nSampleResSta=0;
					vForceLcdRefresh();
				}
			}
			else{
				InputAnalog[N_CANALE_OHMETRO_DYN]=spiralatrice.misUpdate;
				resSta = ((float)spiralatrice.misUpdate) / MAX_FS_ADC_STATICA  * (float)portataDinamica[actStaPortata];
				resSta = (resSta-pTar->o) * pTar->k ;
				spiralatrice.nSampleResSta++;
				spiralatrice.resStaMediataAux+=resSta;
				if (spiralatrice.nSampleResSta==N_SAMPLE_RES_STA){
					spiralatrice.misStaOk=1;
					spiralatrice.resStaMediata=spiralatrice.resStaMediataAux/N_SAMPLE_RES_STA;
					spiralatrice.resStaMediataAux=0.0;
					spiralatrice.nSampleResSta=0;
					vForceLcdRefresh();
					/* Faccio update se � stato rinfrescato tutto il display o se sono nella voce di menu
						che permette il ricalcolo del fattore di correzione
					*/
					if (  (!edit.actLcdChar)
						||((actMenu==MENU_SETUP)&&(actSubmenu==SBM_EB_VISUALIZZA))
						)
						menuOperation(MNU_OP_UPDATE);
				}
				// patch per compensare velocit� incredibile dell'lpc2388: attendo almeno 5ms prima della prox lettura
				misStaSleep=1;
			}
		}
	} /* end if (myInpDig & IDG_MSTA) */


	/* Se c' � una misura statica in corso e l' adc non � occupato a
		  effettuare misure dinamiche...
		Inoltre devo aspettare se sto effettuando un cambio portata...
		Infine se sono nel solito menu diagnostica, sottomenu in/out ana/dig,
		non faccio la misura statica
	*/
	else 


		if (    ( (myInpDig & IDG_MSTA)||ucHoldMisuraStatica)
		   &&(!misStaSleep||(misStaStatus ==MIS_OFF_LINE))
		   &&(!(ucIamInTaraturaDinamica()||ucIamInLogProduzione()||ucIAmSpindleGrinding()))
#ifndef defTest2fili
		   &&!sLCD.ucSuspendDigitalIoUpdate
	       //&&(  (actMenu!=MENU_UTILS)
		   //   ||((actSubmenu!=SBM_UT_INDIG)&&(actSubmenu!=SBM_UT_OUTDIG))
		   //	  )
#endif
	      ){
		/* Lo stato della misura statica e' commutazione multiplexer */
		misStaStatus=MIS_COMM_MUX;
		if (!spiralatrice.StartPortata){
			ucBloccaVisualizzazioneMisuraStatica=0;
			if (spiralatrice.OkAbilitaTaraturaStat)
				ucHoldMisuraStatica=1;
			else
				ucHoldMisuraStatica=0;

			spiralatrice.StartPortata=1;
			// se sono nel menu taratura strumenti e premo misura statica, uso la portata correntemente indicata nel display
			if (ucIamInTaraturaStatica()){
  		       actStaPortata=(NUM_PORTATE-1)-actPortata;
  		   }
  		   else    
				actStaPortata=NUM_PORTATE-1;
			outDigVariable &= 0xfff8;
			outDigVariable |= ODG_MSTA;
			outDigVariable |= ((0x8 >> (actStaPortata)) & 0x7);
			misStaSleep = SLEEP_CHANGE*2;
			goto NonFareMisSta;
		}
		/* Salvo il contatore attuale che indica il numero di cicli di interrupt. */
		m=cntMis;
		spiralatrice.misUpdate =iArrayConvAD7327[0];

		/* Controllo per sicurezza che non sia subentrata un' interruzione... */
		if (!(PrgRunning&&withMis&&(m!=cntMis))){
			// se sono nel menu taratura strumenti e premo misura statica, 
			// continuo ad usare uso la portata correntemente indicata nel display
			// anche se un'altra portata mi garantirebbe miglior accuratezza
			/* Se overflow nella misura, passo alla portata successiva. */
			if( (spiralatrice.misUpdate >= MAX_FS_ADC_STATICA && actStaPortata) 
				&&!(ucIamInTaraturaStatica())
			){
				/* C_ comando fisico */
				--actStaPortata;
				outDigVariable &= 0xfff8;
				outDigVariable |= ODG_MSTA;
				outDigVariable |= ((0x8 >> (actStaPortata)) & 0x7);
				misStaSleep = SLEEP_CHANGE*2;
				/* Indico che il valore di misura statica trovato non � valido. */
				spiralatrice.misStaOk=0;
			}
			/* Se la misura ci sta, ma posso usare una scala che mi garantisce miglior precisione, lo faccio. */
			else if (   (spiralatrice.misUpdate < MIN_FS_ADC_STATICA) 
			         && (actStaPortata < NUM_PORTATE - 1)
				      &&!(ucIamInTaraturaStatica())
			        ) {
				/* C_ comando fisico */
				++actStaPortata;
				outDigVariable &= 0xfff8;
				outDigVariable |= ODG_MSTA;
				outDigVariable |= ((0x8 >> (actStaPortata)) & 0x7);
				misStaSleep = SLEEP_CHANGE*2;
				/* Indico che il valore di misura statica trovato non � valido. */
				spiralatrice.misStaOk=0;
			}
			/* Altrimenti ho una misura che indica morsetti aperti oppure una misura valida. */
			else {
				/* Mi procuro un puntatore al membro dell' array delle
					tarature che viene chiamato in causa. */
				pTar=&macParms.tarParms[N_CANALE_OHMETRO_STA][actStaPortata];
				/*misStaStatus=MIS_OFF_LINE;*/
				// se supero 80% devo cambiare scala...
				// pero' se sono alla portata con pi� capienza, considero che posso arrivare fino al 100%
				if (  (((spiralatrice.misUpdate >= MAX_FS_ADC_STATICA)&&(actStaPortata))||(spiralatrice.misUpdate >= FS_ADC_STATICA))
   				      &&!(ucIamInTaraturaStatica())
				   ){
					spiralatrice.resStaMediata = 2e30;
					spiralatrice.resStaMediataAux=0.0;
					if (++spiralatrice.nSampleResSta>=N_SAMPLE_RES_STA){
						spiralatrice.nSampleResSta=0;
						vForceLcdRefresh();
					}
				}
				else{
					InputAnalog[N_CANALE_OHMETRO_STA]=spiralatrice.misUpdate;
					resSta = (((float)spiralatrice.misUpdate) / MAX_FS_ADC_STATICA) * 2. * (float)portate[actStaPortata];
					resSta = (resSta-pTar->o) * pTar->k ;
					//fCtrlOkResSta[spiralatrice.nSampleResSta]=spiralatrice.misUpdate;
					spiralatrice.nSampleResSta++;
					spiralatrice.resStaMediataAux+=resSta;
					if (spiralatrice.nSampleResSta==N_SAMPLE_RES_STA){
						spiralatrice.misStaOk=1;
						spiralatrice.resStaMediata=spiralatrice.resStaMediataAux/N_SAMPLE_RES_STA;
						spiralatrice.resStaMediataAux=0.0;
						spiralatrice.nSampleResSta=0;
						vForceLcdRefresh();
						//ucHoldMisuraStatica=0;
						/* Faccio update se � stato rinfrescato tutto il display o se sono nella voce di menu
							che permette il ricalcolo del fattore di correzione
						*/
						if (  (!edit.actLcdChar)
							||((actMenu==MENU_SETUP)&&(actSubmenu==SBM_EB_VISUALIZZA))
							)
							menuOperation(MNU_OP_UPDATE);
					}
					// patch per compensare velocit� incredibile dell'lpc2388: attendo almeno 5ms prima della prox lettura
					misStaSleep=1;
				}
			}
		}
	} /* end if (myInpDig & IDG_MSTA) */
	else if (		(spiralatrice.numCanaleUpdate!=N_CANALE_OHMETRO_STA)
				&&(spiralatrice.numCanaleUpdate!=N_CANALE_OHMETRO_DYN)
				&&(spiralatrice.misUpdateStatus==MIS_OFF_LINE)
				&&(misStaStatus==MIS_OFF_LINE)
				&&(!(myInpDig & IDG_MSTA))
				&&(!misStaSleep)
				/* Se sono nel menu diagnostica, sottomenu in/out dig,
					non modifico outdig della portata... */
#ifndef defTest2fili
				&&!sLCD.ucSuspendDigitalIoUpdate
				//&&(  (actMenu!=MENU_UTILS)
				//	||((actSubmenu!=SBM_UT_INDIG)&&(actSubmenu!=SBM_UT_OUTDIG))
				//)
#endif
		   ){
		spiralatrice.StartPortata=0;
		spiralatrice.misStaOk=0;
		outDigVariable &= ~ODG_MSTA;
		outDigVariable &= 0xfff8;
	}
	if (!(myInpDig & IDG_MSTA)&&!ucHoldMisuraStatica){
		// se avevo attivato la misura dinamica
		// reimposto la portata dell'ohmetro dinamico portandola 
		// al valore massimo, ovviamente se non c'� un programma in esecuzione
		if (sLCD.ucAttivataMisuraDinamica){
			sLCD.ucAttivataMisuraDinamica=0;
			if (!PrgRunning){
				// Mi rimetto con la portata piu' alta, che assicura la corrente minima sulle torrette. 
				vUpdateDacPortValue((enum_select_dac_portata)0);	
			}
		}
		spiralatrice.OkMisStatica=1;
		spiralatrice.StartPortata=0;
		misStaStatus =MIS_OFF_LINE;
		// se tasto non premuto, ripristino la visualizzazione della misura statica
		ucBloccaVisualizzazioneMisuraStatica=0;

		spiralatrice.misStaOk=0;
		spiralatrice.nSampleResSta=0;
		spiralatrice.resStaMediataAux=0.0;
		spiralatrice.resStaMediata=2e30;

	}
NonFareMisSta:;
}








/* Conversione ascii- floating di n caratteri. */
float mynatof(char *p, unsigned char n){
	xdat float      fract = 0.1;
	xdat float      retAtof = 0.;
	xdat unsigned char exp;

	exp = 1;

	while (isdigit(*p)&&n--) {
		retAtof = retAtof * 10.0 + (*p++ - '0');
		exp = 0;
	}

	if(*p++ == '.' &&n--) {
		while(isdigit(*p)&&n--) {
			retAtof = fract * (*p++ - '0') + retAtof;
			fract *= 0.1;
			exp = 0;
		}
	}

	if(exp)
		retAtof = 0.;

   return retAtof;
}














/* Funzione che serve per creare la rampa di aggiornamento uscite dac.
   I valori delle uscite vanno scritti nell' array spiralatrice.DacOutValue[];
   i valori MASSIMI delle uscite vanno scritti nell' array spiralatrice.DacOutMaxValue[];
   la funzione provvede ad effettuare il clipping al fondo scala dell' adc.
   Ingresso: l' indirizzo del dac cui spedire il dato.
   Il valore in uscita dal dac viene aggiornato solo se il valore attuale
   � diverso da quello proposto!
   Uscita: nulla.
*/

void SetDacValue(unsigned char DacOutIndex){
	unsigned char xdat i,bitRampaDac;
	bitRampaDac=1<<DacOutIndex;
	/* Moltiplico per due l' indirizzo in modo da ottenere il numero
	valido dell' indice dell' array che contiene le uscite dei dac. */
	DacOutIndex<<=1;
	i=0;
	while (i<NUM_FRIZIONI){
		/* La trasformo in comando per il dac. */
		spiralatrice.DacOutValue_dacstep[i]=((((long)spiralatrice.DacOutValue[i])*FS_DAC_MAX519)/spiralatrice.DacOutMaxValue[i]);
		/* Saturazione al fondo scala dell' adc. */
		if (spiralatrice.DacOutValue_dacstep[i]>FS_DAC_MAX519)
			spiralatrice.DacOutValue_dacstep[i]=FS_DAC_MAX519;
		/* Il comando al dac viene dato solo se il valore attuale � diverso da quello gi� presente! */
		// il comando viene sempre inviato se � un azzeramento
		if ((spiralatrice.DacOut[DacOutIndex]!=spiralatrice.DacOutValue_dacstep[i])||(spiralatrice.DacOutValue_dacstep[i]==0)){
			/* Setto il flag che stabilisce quale DAC necessita di rampa. */
			spiralatrice.StatusRampaDac|=bitRampaDac;
			/* Indico di partire dal primo gradino della rampa. */
			spiralatrice.GradinoRampaDAC=0;
		}
		/* Inserisco i valori delle uscite nell' array apposito. */
		spiralatrice.DacOut[DacOutIndex]=(unsigned char)spiralatrice.DacOutValue_dacstep[i];
		/* Passo al prossimo elemento dell' array. */
		DacOutIndex++;
		i++;
	}
}//static void SetDacValue(unsigned char DacOutIndex)

//
//
//  routines di gestione della commessa
//
//
/* Routine che cancella la commessa indicata.
   La struttura della commessa viene sbiancata.
   Una commessa vuota pu� essere riconosciuta dal fatto che il suo
   numero (campo numcommessa) vale 0.
*/
void DeleteCommessa(unsigned int nCommessa){
	COMMESSA xdat * xdat pCommessa;
	unsigned char xdat i;

	pCommessa=&nvram_struct.commesse[nCommessa];
	pCommessa->commessa[0]=0x00;
	pCommessa->numcommessa=0;
	pCommessa->uiValidComm_0xa371=0;
	// azzero anche il numero di pezzi in maggiorazione
	pCommessa->magg=0;
	pCommessa->ucWorkingMode_0L_1LR=0;
	pCommessa->numprogramma=0;
	pCommessa->n=0;
	pCommessa->res=0.0;
	pCommessa->resspec_spir=0;
	i=0;
	while (i<NUM_FRIZIONI){
		pCommessa->assorb_frizioni[i]=0;
		pCommessa->vruote_comm[i]=0;
		i++;
	}
	pCommessa->rpm=0;
	pCommessa->automatico=0;
	pCommessa->defPortata=0;
	i=pCommessa->prevComm;
	/* Se sono la prima commessa, sposto il puntatore a commessa. */
	/* <---- modifica del 07/04/1997 */
	if (i==0){
		/* Se non c' � una commessa successiva, lo indico,
		altrimenti sposto il puntatore alla prima commessa. */

		if (pCommessa->nextComm==0)
			spiralatrice.firstComm=0;
		else{
			i=pCommessa->nextComm;
			spiralatrice.firstComm=i;
			nvram_struct.commesse[i-1].prevComm=0;
		}
	}
	else{
		/* Esiste una commessa precedente, ne aggiorno il puntatore
		alla prossima. */
		nvram_struct.commesse[i-1].nextComm=pCommessa->nextComm;
		/* Vedo se esiste una commessa successiva. */
		i=pCommessa->nextComm;
		/* Se esiste, ne aggiorno il puntatore all' indietro. */
		if (i){
			nvram_struct.commesse[i-1].prevComm=pCommessa->prevComm;
		}
	}
}


/* Programma di ricerca prossima commessa in lista.
   Ingresso: numero nella tabella delle nvram_struct.commesse della commessa attuale.
   Se la commessa attuale e' l' unica in lista ne restituisce il numero.
*/
unsigned char SearchNextCommessa(unsigned char from){
 unsigned char xdat i;
 i=nvram_struct.commesse[from].nextComm;
 return (i)?i-1:from;
}


/* Programma di ricerca precedente commessa in lista.
   Ingresso: numero nella tabella delle nvram_struct.commesse della commessa attuale.
   Se la commessa attuale e' l' unica in lista ne restituisce il numero.
*/
unsigned char SearchPrevCommessa(unsigned char from){
 unsigned char xdat i;
 i=nvram_struct.commesse[from].prevComm;
 /* <------ Modifica del 07/04/1997 */
 /*era: return (i)?i-1:from;*/
 return (i)?i-1:from;
}



/*                                         */
/*                                         */
/* Funzioni di gestione stringhe.          */
/*                                         */
/*                                         */




// variabile che contiene le strutture per la gestione delle finestre...
xdata TipoHandleWindow hw;


void vJumpToWindow(tipoIdWindow id){
	unsigned int uiSP,uiWinIdx;
	uiSP=hw.uiSP;
	uiWinIdx=hw.uiStack[uiSP];
	hw.uiIdJumper=uiWinIdx;
	hw.win[uiWinIdx].status=enumWindowStatus_Null;
	hw.uiStack[uiSP]=id;
	hw.win[id].status=enumWindowStatus_Initialize;
}

unsigned char ucIsEmptyStackCaller(void){
	return hw.uiSP?0:1;
}


// svuoto stack
// metto in prima posizione id finestra corrente...
void vEmptyStackCaller(void){
	unsigned int uiSP;
	uiSP=hw.uiSP;
	hw.uiStack[0]=hw.uiStack[uiSP];
	hw.uiSP=0;
}

unsigned char ucCallWindow(tipoIdWindow id){
	unsigned int uiSP,uiWinIdx;
	uiSP=hw.uiSP;
	if (uiSP>=defMaxWindow-1)
		return 0;
	uiWinIdx=hw.uiStack[uiSP];
	// salvo id del caller
	hw.uiIdCaller=uiWinIdx;
	// esco dalla finestra, passo a null...
	hw.win[uiWinIdx].status=enumWindowStatus_WaitingReturn;
	++hw.uiSP;
	uiSP=hw.uiSP;
	hw.uiStack[uiSP]=id;
	hw.win[id].status=enumWindowStatus_Initialize;
	return 1;
}//unsigned char ucCallWindow(tipoIdWindow id)

unsigned char ucReturnFromWindow(){
	xdata tipoIdWindow id;
	unsigned int uiSP;
	uiSP=hw.uiSP;
	id=hw.uiStack[uiSP];
	// salvo l'id del returner
	hw.uiIdReturner=id;
	// esco dalla finestra, passo a null...
	hw.win[id].status=enumWindowStatus_Null;
	if (hw.uiSP<1)
		return 0;
	// arretro di 1 con l'indice stack pointer, per cui punto alla finestra chiamante
	--hw.uiSP;
	uiSP=hw.uiSP;
	id=hw.uiStack[uiSP];
	// ritorno alla finestra, la devo inizializzare...
	// cio� creare buttons etc
	hw.win[id].status=enumWindowStatus_ReturnFromCall;
	return 1;
}

// preleva l'id della finestra corrente
tipoIdWindow uiGetIdCurrentWindow(void){
	unsigned int uiSP;
	uiSP=hw.uiSP;
	return hw.uiStack[uiSP];
}//tipoIdWindow uiGetIdCurrentWindow

// restituisce lo status della finestra corrente
enumWindowStatus uiGetStatusCurrentWindow(void){
	unsigned int uiSP,uiWinIdx;
	uiSP=hw.uiSP;
	uiWinIdx=hw.uiStack[uiSP];
	return hw.win[uiWinIdx].status;
}//tipoIdWindow 

// restituisce lo status della finestra corrente
void vSetStatusCurrentWindow(enumWindowStatus status){
	unsigned int uiSP,uiWinIdx;
	uiSP=hw.uiSP;
	uiWinIdx=hw.uiStack[uiSP];
	hw.win[uiWinIdx].status=status;
}//tipoIdWindow 


// impostazione di un parametro di una finestra
unsigned char ucSetWindowParam(tipoIdWindow id,unsigned char ucParamIdx, tipoParamWindow uiParamValue){
	if ((id>=defMaxWindow)||(ucParamIdx>=defMaxWindowParams))
		return 0;
	hw.win[id].uiParams[ucParamIdx]=uiParamValue;
	return 1;
}

// legge un parametro di una win 
tipoParamWindow uiGetWindowParam(tipoIdWindow id,unsigned char ucParamIdx){
	if ((id>=defMaxWindow)||(ucParamIdx>=defMaxWindowParams))
		return 0;
	return hw.win[id].uiParams[ucParamIdx];
}
// legge un parametro della win attiva
tipoParamWindow uiGetActiveWindowParam(unsigned char ucParamIdx){
	unsigned int uiSP,uiWinIdx;
	if (ucParamIdx>=defMaxWindowParams)
		return 0;
	uiSP=hw.uiSP;
	uiWinIdx=hw.uiStack[uiSP];
	return hw.win[uiWinIdx].uiParams[ucParamIdx];
}
// scrive  un parametro della win attiva
void vSetActiveWindowParam(unsigned char ucParamIdx,tipoParamWindow uiParamValue){
	unsigned int uiSP,uiWinIdx;
	if (ucParamIdx>=defMaxWindowParams)
		return;
	uiSP=hw.uiSP;
	uiWinIdx=hw.uiStack[uiSP];
	hw.win[uiWinIdx].uiParams[ucParamIdx]=uiParamValue;
}


// copia stringa al max n caratteri aggiungendo blank a destra se servono
void vStrcpy_padBlank(unsigned char xdata *pdst,unsigned char xdata *psrc,unsigned char ucNumChar){
	xdata unsigned char i;
	i=0;
	while (i<ucNumChar){
		if (!*psrc)
			break;
		*pdst++=*psrc++;
		i++;
	}
	while (i<ucNumChar){
		*pdst++=' ';
		i++;
	}
	*pdst=0;
}

// riempie la stringa di n caratteri blank
void vStr_fillBlank(unsigned char xdata *pdst,unsigned char ucNumChar){
	xdata unsigned char i;
	i=0;
	while (i<ucNumChar){
		*pdst++=' ';
		i++;
	}
	*pdst=0;
}

// mette in uppercase la stringa
void vStrUpperCase(unsigned char xdata *pdst){
	while(*pdst){
		*pdst=toupper(*pdst);
		pdst++;
	}
}


// copia stringa al max n caratteri togliendo blank a destra e sinsitra
void vStrcpy_trimBlank(unsigned char xdata *pdst,unsigned char xdata *psrc,unsigned char ucNumChar){
	xdata unsigned char i;
	unsigned char xdata *paux;
	// inizializzo a null
	*pdst=0;
	i=0;
	while (i<ucNumChar){
		if (!*psrc){
			return;
		}
		if (*psrc!=' ')
			break;
		psrc++;
		i++;
	}
	// vado alla fine della stringa
	paux=psrc;
	while(*paux)
		paux++;
	// risalgo eliminando i caratteri in coda
	while(--paux>psrc){
		if (*paux!=' ')
			break;
	}

	while(i<ucNumChar){
		if (!*psrc||(psrc>paux))
			break;
		*pdst++=*psrc++;
		i++;
	}
	*pdst=0;
}



unsigned char ucIsUmSensitive(enumUmType um){
	switch(um){
		case enumUm_mm_giro:
		case enumUm_metri_minuto:
		case enumUm_mm:
		case enumUm_ohm_per_metro:
		case enumUm_inch_giro:
		case enumUm_feet_minuto:
		case enumUm_inch:
		case enumUm_inch_feet:
		case enumUm_ohm_per_feet:
			return 1;
		default:
		{
			break;
		}
	}
	return 0;
}

code unsigned char tableUm[enumUm_NumOf][defMaxCharUmNumK+1]=
{
	 "None",
	 "Imp/Giro",
	 "mm/Giro",
	 "inch/Giro",
	 "Rpm",
	 "mt/min",
	 "ft/min",
	 "A",  // Amp�re 
	 "mm",
	 "inch",
	 "inch-feet",
	 "N.",
	 "Ohm",
	 "ohm/mt",
	 "ohm/feet",
	 "%",
	 "Secondi", /* Secondi. */
	 "Secondi/1000" /* MilliSecondi. */

};

TipoStructParamsNumK xdata paramsNumK;

extern unsigned char ucCtrlMinMaxValue(xdata unsigned char *pValue);


typedef struct _TipoStructCodesT9{
	unsigned char ucNumOf;
	unsigned char ucChars[4+1];
}TipoStructCodesT9;

code TipoStructCodesT9 ucCodes_T9[]={
	{1," "},
	{3,"ABC"},
	{3,"DEF"},
	{3,"GHI"},
	{3,"JKL"},
	{3,"MNO"},
	{4,"PQRS"},
	{3,"TUV"},
	{4,"WXYZ"},
	{4,"_@-,"},
	{1,"."},
};


unsigned char ucT9_handle_button_pressed(unsigned char ucButton){
	paramsNumK.ucT9_found=0;
	paramsNumK.ucT9_pressedButton=1;
	paramsNumK.ucT9_numLoop=0;
	// se cambiato button
	if (paramsNumK.ucT9_lastButton!=ucButton){
		if (paramsNumK.ucT9_lastButton!=0xFF){
			// inserisci testo ultimo selezionato
			paramsNumK.ucT9_found=ucCodes_T9[paramsNumK.ucT9_lastButton].ucChars[paramsNumK.ucT9_lastCharIdx];
		}
		paramsNumK.ucT9_lastCharIdx=0;
	}
	else{
		if (++paramsNumK.ucT9_lastCharIdx>=ucCodes_T9[paramsNumK.ucT9_lastButton].ucNumOf){
			paramsNumK.ucT9_lastCharIdx=0;
		}
	}
	paramsNumK.ucT9_lastButton=ucButton;
	paramsNumK.ucT9_candidate=ucCodes_T9[paramsNumK.ucT9_lastButton].ucChars[paramsNumK.ucT9_lastCharIdx];

	return paramsNumK.ucT9_found;
}


// restituisce 1 se il carattere di picture NumK ammette solo le digit 1..9
unsigned char ucPictureNumK_OnlyDigit_1_9_Ok(unsigned char ucIdx){
	if (!paramsNumK.ucPictureEnable)
		return 0;
	switch(paramsNumK.ucPicture[ucIdx]){
		case 'S':
		case 's':
			return 1;
	}
	return 0;
}//unsigned char ucPictureNumK_OnlyDigit_1_9_Ok(unsigned char uc)




// restituisce 1 se il carattere di picture NumK ammette le digit 0..9
unsigned char ucPictureNumK_DigitOk(unsigned char ucIdx){
	if (!paramsNumK.ucPictureEnable)
		return 1;
	switch(paramsNumK.ucPicture[ucIdx]){
		case 'n':
		case 'S':
		case 's':
		case 'N':
		case 'x':
		case 'p':
			return 1;
	}
	return 0;
}//unsigned char ucPictureNumK_DigitOk(unsigned char uc)

// restituisce 1 se il carattere di picture NumK ammette il punto decimale
unsigned char ucPictureNumK_DecimalPointOk(unsigned char ucIdx){
	if (!paramsNumK.ucPictureEnable)
		return 1;
	switch(paramsNumK.ucPicture[ucIdx]){
		case 'x':
		case 'p':
			return 1;
		case '.':
		case 'N':
			if (!paramsNumK.ucDotInserted)
				return 1;
			return 0;
	}
	return 0;
}//unsigned char ucPictureNumK_DigitOk(unsigned char uc)

unsigned char ucPictureNumK_IsNoModify(unsigned char ucIdx){
	if (!paramsNumK.ucPictureEnable)
		return 0;
	switch(paramsNumK.ucPicture[ucIdx]){
		case '.':
			return 1;
	}
	return 0;
}//unsigned char ucPictureNumK_IsNoModify(unsigned char ucIdx)


// conta il numero di caratteri significativi (max 1024) contenuti in una stringa password,
// che non pu� iniziare per spazio o zero, 
// quindi deve essere un numeor che ha esattamente 5 caratteri
unsigned int uiCountEffectiveCharsInPasswordString(char *s){
	long lAux;
	// converto stringa in numero
	lAux=atol(s);
	// password � valida se contiene 5 chars significativi
	// il primo carattere deve essere diverso da zero
	if ((lAux>=10000UL)&&(lAux<=99999UL)){
		return 5;
	}
	// se password non � valida, ritorno zero caratteri
	return 0;
}//unsigned int uiCountEffectiveCharsInPasswordString(char *s)

// gestione numeric keypad
// se ritorna 2--> richiama ancora una volta la gestione finestre, c'� stato un cambio di finestra
unsigned char ucHW_Num_Keypad(void){
	#define defMaxCharTextNumKeypad 32
	typedef struct _TipoStructNumK{
		unsigned char row, col;
		unsigned char stringa[defMaxCharTextNumKeypad+1];
		enumBitmap_40x24_Type bitmap_type;
	}TipoStructNumK;


	#define defOffsetXButtonsNumKeypad 52

	#define defColStartNumKeypad ((defLcdWidthX_pixel-(defOffsetXButtonsNumKeypad*3+40))/2)
	#define defColStartNumKeypad_value 54

	#define defRowStartNumKeypad 44
	#define defRowStartNumKeypad_value 48

	#define defOffsetYButtonsNumKeypad 36

	#define defNumK_TitleRow 8
	#define defNumK_TitleCol defColStartNumKeypad

	xdata unsigned int uiColStartNumKeypad_value;

	static code TipoStructNumK numk_buttons[]={
		{defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad, 0+defColStartNumKeypad,"0",enumBitmap_40x24_0},
		{defRowStartNumKeypad+defOffsetYButtonsNumKeypad ,                            0+defColStartNumKeypad,"1",enumBitmap_40x24_1},
		{defRowStartNumKeypad+defOffsetYButtonsNumKeypad ,   defOffsetXButtonsNumKeypad+defColStartNumKeypad,"2",enumBitmap_40x24_2},
		{defRowStartNumKeypad+defOffsetYButtonsNumKeypad , 2*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"3",enumBitmap_40x24_3},
		{defRowStartNumKeypad+2*defOffsetYButtonsNumKeypad,                           0+defColStartNumKeypad,"4",enumBitmap_40x24_4},
		{defRowStartNumKeypad+2*defOffsetYButtonsNumKeypad,  defOffsetXButtonsNumKeypad+defColStartNumKeypad,"5",enumBitmap_40x24_5},
		{defRowStartNumKeypad+2*defOffsetYButtonsNumKeypad,2*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"6",enumBitmap_40x24_6},
		{defRowStartNumKeypad+3*defOffsetYButtonsNumKeypad,                           0+defColStartNumKeypad,"7",enumBitmap_40x24_7},
		{defRowStartNumKeypad+3*defOffsetYButtonsNumKeypad,  defOffsetXButtonsNumKeypad+defColStartNumKeypad,"8",enumBitmap_40x24_8},
		{defRowStartNumKeypad+3*defOffsetYButtonsNumKeypad,2*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"9",enumBitmap_40x24_9},
		{defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad,1*defOffsetXButtonsNumKeypad+defColStartNumKeypad," . ",enumBitmap_40x24_dot},
        {defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad,2*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"W",enumBitmap_40x24_abc},
		{defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad,3*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"OK ",enumBitmap_40x24_OK},
		{defRowStartNumKeypad+defOffsetYButtonsNumKeypad , 3*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"Esc",enumBitmap_40x24_ESC},
		{defRowStartNumKeypad+2*defOffsetYButtonsNumKeypad,3*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"Bks",enumBitmap_40x24_backspace},
		{defRowStartNumKeypad+3*defOffsetYButtonsNumKeypad,3*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"-->",enumBitmap_40x24_cursorRight},
	};
	
	static code TipoStructNumK numk_buttons_T9[]={
		{defRowStartNumKeypad+defOffsetYButtonsNumKeypad ,                            0+defColStartNumKeypad," ",enumBitmap_40x24_space},
		{defRowStartNumKeypad+defOffsetYButtonsNumKeypad ,   defOffsetXButtonsNumKeypad+defColStartNumKeypad,"A",enumBitmap_40x24_abc},
		{defRowStartNumKeypad+defOffsetYButtonsNumKeypad , 2*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"D",enumBitmap_40x24_def},
		{defRowStartNumKeypad+2*defOffsetYButtonsNumKeypad,                           0+defColStartNumKeypad,"G",enumBitmap_40x24_ghi},
		{defRowStartNumKeypad+2*defOffsetYButtonsNumKeypad,  defOffsetXButtonsNumKeypad+defColStartNumKeypad,"J",enumBitmap_40x24_jkl},
		{defRowStartNumKeypad+2*defOffsetYButtonsNumKeypad,2*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"M",enumBitmap_40x24_mno},
		{defRowStartNumKeypad+3*defOffsetYButtonsNumKeypad,                           0+defColStartNumKeypad,"P",enumBitmap_40x24_pqrs},
		{defRowStartNumKeypad+3*defOffsetYButtonsNumKeypad,  defOffsetXButtonsNumKeypad+defColStartNumKeypad,"T",enumBitmap_40x24_tuv},
		{defRowStartNumKeypad+3*defOffsetYButtonsNumKeypad,2*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"W",enumBitmap_40x24_wxyz},
	
		{defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad,0*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"@",enumBitmap_40x24_other_chars},

		{defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad,1*defOffsetXButtonsNumKeypad+defColStartNumKeypad," . ",enumBitmap_40x24_dot},

		{defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad,2*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"123",enumBitmap_40x24_123},
	
		{defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad,3*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"OK ",enumBitmap_40x24_OK},
		{defRowStartNumKeypad+defOffsetYButtonsNumKeypad , 3*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"Esc",enumBitmap_40x24_ESC},
		{defRowStartNumKeypad+2*defOffsetYButtonsNumKeypad,3*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"Bks",enumBitmap_40x24_backspace},
		{defRowStartNumKeypad+3*defOffsetYButtonsNumKeypad,3*defOffsetXButtonsNumKeypad+defColStartNumKeypad,"-->",enumBitmap_40x24_cursorRight},
	
	};
	

	typedef enum {
		enumBtnNumK_0,
		enumBtnNumK_1,
		enumBtnNumK_2,
		enumBtnNumK_3,
		enumBtnNumK_4,
		enumBtnNumK_5,
		enumBtnNumK_6,
		enumBtnNumK_7,
		enumBtnNumK_8,
		enumBtnNumK_9,
		enumBtnNumK_Dot,
		enumBtnNumK_goto_t9,
		enumBtnNumK_OK,
		enumBtnNumK_Exit,
		enumBtnNumK_Back,
		enumBtnNumK_right,
		enumBtnNumK_TheNumber,
		enumBtnNumK_Title,
		enumBtnNumK_Um,
		enumBtnNumK_NumOf_normal=enumBtnNumK_Um,
		enumBtnNumK_A,
		enumBtnNumK_B,
		enumBtnNumK_C,
		enumBtnNumK_D,
		enumBtnNumK_E,
		enumBtnNumK_F,
		enumBtnNumK_NumOf
	}enumButtons_Num_Keypad;

	typedef enum {
		enumBtnNumK_t9_space,
		enumBtnNumK_t9_abc,
		enumBtnNumK_t9_def,
		enumBtnNumK_t9_ghi,
		enumBtnNumK_t9_jkl,
		enumBtnNumK_t9_mno,
		enumBtnNumK_t9_pqrs,
		enumBtnNumK_t9_tuv,
		enumBtnNumK_t9_wxyz,
		enumBtnNumK_t9_other_chars,
		enumBtnNumK_t9_dot,
		enumBtnNumK_t9_goto_alfa,
		enumBtnNumK_OK_t9,
		enumBtnNumK_Exit_t9,
		enumBtnNumK_Back_t9,
		enumBtnNumK_right_t9,
		enumBtnNumK_TheNumber_t9,
		enumBtnNumK_Title_t9,
		enumBtnNumK_Um_t9,
		enumBtnNumK_NumOf_t9
	}enumButtons_Num_Keypad_t9;

	// finestra Numeric keypad: parametro che definisce posizione cursore
	#define NumK_CursorPos 0
	// finestra Numeric keypad: parametro che dice se siamo appena entrati nella finestra
	#define NumK_JustEntered 1
	// finestra info: serve per avvisi del tipo: digitare ancora la password, password corretta, passowrd errata
	#define NumK_CallInfoWindow 2
	// numero di loop epr accettazione dato t9
	#define defMinLoopT9_ValidChar 8

	xdata unsigned char i,ucCursorPos;
	xdata enumFontType fontButton;
	xdata unsigned int uiColonna;
	xdata static unsigned int uiSaveTimeoutLcd_NumK;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			switch (defGetWinParam(NumK_CallInfoWindow)){
				// confirm password 
				case 0:
					vSetStatusCurrentWindow(enumWindowStatus_Active);
					break;
				// ok password 
				case 1:
				// bad password 
				case 2:
					ucReturnFromWindow();
					return 2;
					break;
			}
			
			return 2;
		case enumWindowStatus_Initialize:
			uiSaveTimeoutLcd_NumK=uiLcdRefreshTimeout_GetAct();
			i=0;
			while (i<enumBtnNumK_NumOf){
				ucCreateTheButton(i); 
				i++;
			}
			paramsNumK.ucNumLoopPassword=0;
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			// posizione cursore=0
			defSetWinParam(NumK_CursorPos,0);
			// siamo appena entrati nella win
			defSetWinParam(NumK_JustEntered,1);
			// in ingresso alla finestra, t9 non deve essere abilitato
			paramsNumK.ucT9_enabled=0;
			// controllo che numero max caratteri sia ok, per evitare di sporcare la memoria
			if (paramsNumK.ucMaxNumChar>defMaxCharTextNumKeypad)
				paramsNumK.ucMaxNumChar=defMaxCharTextNumKeypad;
			// copio in stringa_out la stringa che viene passata in ingresso
			vStrcpy_padBlank(hw.ucStringNumKeypad_out,hw.ucStringNumKeypad_in,paramsNumK.ucMaxNumChar);
			// posto il fuoco fino al primo carattere modificabile!
			i=0;
			while(i<paramsNumK.ucMaxNumChar-1){
				if (ucPictureNumK_IsNoModify(i)){
					// Sposto il cursore
					defSetWinParam(NumK_CursorPos,i+1);
					// non siamo appena entrati!
					defSetWinParam(NumK_JustEntered,0);
					if (hw.ucStringNumKeypad_out[i]=='.')
						paramsNumK.ucDotInserted=1;
				}
				i++;
			}

			return 2;
		case enumWindowStatus_Active:
			// indico se al momento il dot � o non � stato digitato
			if (strchr(hw.ucStringNumKeypad_out,'.'))
				paramsNumK.ucDotInserted=1;
			else
				paramsNumK.ucDotInserted=0;
			// posizione del cursore...
			ucCursorPos=defGetWinParam(NumK_CursorPos);
			i=0;
			while(i<enumBtnNumK_NumOf){
				if (ucHasBeenPressedButton(i)){
					if (    paramsNumK.ucT9_enabled
						 && ((i>=enumBtnNumK_t9_space)&&(i<=enumBtnNumK_t9_dot))
						){
						ucT9_handle_button_pressed(i);
					}
					else if (!paramsNumK.ucT9_enabled&&(i>=enumBtnNumK_0)&&(i<=enumBtnNumK_9)){
						// se solo cifre '1'..'9' ammesse e premuto '0', passo oltre
						if (ucPictureNumK_OnlyDigit_1_9_Ok(ucCursorPos)&&(i==enumBtnNumK_0)){
							continue;
						}

						// posso qui inserire una cifra???
						if (!ucPictureNumK_DigitOk(ucCursorPos))
							continue;
						// se � il primo tasto che premiamo... la stringa si deve resettare
						if (defGetWinParam(NumK_JustEntered)==1){
							// reset stringa_out
							vStr_fillBlank(hw.ucStringNumKeypad_out,paramsNumK.ucMaxNumChar);
							// posizione cursore=0
							defSetWinParam(NumK_CursorPos,0);
						}
						// per prudenza, se sono entro la parte lecita, assegno il carattere
						if (ucCursorPos<paramsNumK.ucMaxNumChar){
							hw.ucStringNumKeypad_out[ucCursorPos]=i+'0';
						}
						// sposto cursore se possibile
						if (ucCursorPos<paramsNumK.ucMaxNumChar-1){
							// Sposto il cursore
							defSetWinParam(NumK_CursorPos,++ucCursorPos);
						}
					}
					
					else if (uc_use_hexadecimal_letters&&(i>=enumBtnNumK_A)&&(i<=enumBtnNumK_F)){
						// se � il primo tasto che premiamo... la stringa si deve resettare
						if (defGetWinParam(NumK_JustEntered)==1){
							// reset stringa_out
							vStr_fillBlank(hw.ucStringNumKeypad_out,paramsNumK.ucMaxNumChar);
							// posizione cursore=0
							defSetWinParam(NumK_CursorPos,0);
						}
						// per prudenza, se sono entro la parte lecita, assegno il carattere
						if (ucCursorPos<paramsNumK.ucMaxNumChar){
							hw.ucStringNumKeypad_out[ucCursorPos]=i-enumBtnNumK_A+'A';
						}
						// sposto cursore se possibile
						if (ucCursorPos<paramsNumK.ucMaxNumChar-1){
							// Sposto il cursore
							defSetWinParam(NumK_CursorPos,++ucCursorPos);
						}
					}
					else switch(i){
						case enumBtnNumK_Exit:
							uc_use_hexadecimal_letters=0;
							paramsNumK.ucEnableT9=0;
							paramsNumK.ucT9_enabled=0;
							// recupero la stringa originale...
							// eseguo il trim...
							vStrcpy_padBlank(hw.ucStringNumKeypad_out,hw.ucStringNumKeypad_in,paramsNumK.ucMaxNumChar);
							ucLcdRefreshTimeoutEnable(uiSaveTimeoutLcd_NumK);
							ucReturnFromWindow();
							return 2;
						case enumBtnNumK_OK:
							uc_use_hexadecimal_letters=0;
						
							ucLcdRefreshTimeoutEnable(uiSaveTimeoutLcd_NumK);
							// se t9 aveva un carattere, lo salvo
							if (    paramsNumK.ucT9_enabled
								 && paramsNumK.ucT9_candidate
								 &&(paramsNumK.ucT9_candidate!=0xFF)
								 &&(ucCursorPos<paramsNumK.ucMaxNumChar)
							   ){
								hw.ucStringNumKeypad_out[ucCursorPos]=paramsNumK.ucT9_candidate;
								paramsNumK.ucT9_candidate=0xFF;
								paramsNumK.ucT9_lastButton=0xFF;
							}
							paramsNumK.ucEnableT9=0;
							paramsNumK.ucT9_enabled=0;
							if (ucCursorPos>=paramsNumK.ucMaxNumChar)
								hw.ucStringNumKeypad_out[ucCursorPos]=0;
							// controllo min/max, per ora non attivo...
							ucCtrlMinMaxValue(hw.ucStringNumKeypad_out);
							// la password deve contenere almeno 5 caratteri!!!!
							if ((paramsNumK.enumUm==enumUm_password)||(paramsNumK.enumUm==enumUm_modify_password)){
								if (uiCountEffectiveCharsInPasswordString(hw.ucStringNumKeypad_out)<5)
									break;
							}
							
							if (  (paramsNumK.enumUm==enumUm_modify_password)
//								||(paramsNumK.enumUm==enumUm_password)
							   ){
								if (++paramsNumK.ucNumLoopPassword==1){
									paramsNumK.ulPasswordEntered=atol(hw.ucStringNumKeypad_out);
									if (paramsNumK.enumUm==enumUm_modify_password){
										strcpy(hw.ucString,"  ");
										// eseguo il pad
										vStrcpy_padBlank(hw.ucStringNumKeypad_out,hw.ucString,paramsNumK.ucMaxNumChar);
										// posizione cursore=0
										defSetWinParam(NumK_CursorPos,0);
										// messaggio: ridigitare la password
										defSetWinParam(NumK_CallInfoWindow,0);
										// numero msg da visualizzare: 1
										ucSetWindowParam(enumWinId_Info,0, 1);
										// messaggio da visualizzare: retype password
										ucSetWindowParam(enumWinId_Info,1, enumStr20_retype_password);
										// chiamo la finestra info
										ucCallWindow(enumWinId_Info);
										return 2;
									}
								}
								if (paramsNumK.ulPasswordEntered==atol(hw.ucStringNumKeypad_out)){
									// numero msg da visualizzare: 1
									ucSetWindowParam(enumWinId_Info,0, 1);
									// messaggio: password ok
									defSetWinParam(NumK_CallInfoWindow,1);
									// messaggio da visualizzare: password ok
									ucSetWindowParam(enumWinId_Info,1, enumStr20_ok_password);
								    // chiamo la finestra info
							        ucCallWindow(enumWinId_Info);
								}
								else{
									// recupero la password
									vStrcpy_padBlank(hw.ucStringNumKeypad_out,hw.ucStringNumKeypad_in,paramsNumK.ucMaxNumChar);
									// numero msg da visualizzare: 1
									ucSetWindowParam(enumWinId_Info,0, 1);
									// messaggio da visualizzare: password bad
									ucSetWindowParam(enumWinId_Info,1, enumStr20_bad_password);
									// messaggio: password KO
									defSetWinParam(NumK_CallInfoWindow,2);
								    // chiamo la finestra info
							        ucCallWindow(enumWinId_Info);
								}
								return 2;
							}

							// copio in "in" la stringa modificata
							mystrcpy(hw.ucStringNumKeypad_in,hw.ucStringNumKeypad_out,sizeof(hw.ucStringNumKeypad_in)-1);
							// eseguo il pad
							vStrcpy_padBlank(hw.ucStringNumKeypad_out,hw.ucStringNumKeypad_in,paramsNumK.ucMaxNumChar);
							ucReturnFromWindow();
							return 2;
						case enumBtnNumK_right:
							if (    paramsNumK.ucT9_enabled
								 && paramsNumK.ucT9_candidate
								 &&(paramsNumK.ucT9_candidate!=0xFF)
								 &&(ucCursorPos<paramsNumK.ucMaxNumChar)
							   ){
								hw.ucStringNumKeypad_out[ucCursorPos]=paramsNumK.ucT9_candidate;
								paramsNumK.ucT9_candidate=0xFF;
								paramsNumK.ucT9_lastButton=0xFF;
							}
							// se sono su uno dei caratteri centrali, oppure sono sull'ultimo, ma l'ultimo � blank,
							// allora rendo blank il carattere precedente
							if (  (ucCursorPos<paramsNumK.ucMaxNumChar-1)
								&&(ucCursorPos<strlen(hw.ucStringNumKeypad_out))
							   ){
								// Sposto il cursore
								++ucCursorPos;
								defSetWinParam(NumK_CursorPos,ucCursorPos);
							}
							break;
						case enumBtnNumK_Back:
							paramsNumK.ucT9_candidate=0xFF;
							paramsNumK.ucT9_lastButton=0xFF;
							// se � il primo tasto che premiamo... la stringa si deve resettare
							if (defGetWinParam(NumK_JustEntered)==1){
								// reset stringa_out
								vStr_fillBlank(hw.ucStringNumKeypad_out,paramsNumK.ucMaxNumChar);
								// posizione cursore=0
								defSetWinParam(NumK_CursorPos,0);
							}
							if (ucCursorPos){
								// se sono su uno dei caratteri centrali, oppure sono sull'ultimo, ma l'ultimo � blank,
								// allora rendo blank il carattere precedente
								if (  (ucCursorPos<paramsNumK.ucMaxNumChar-1)
									||(hw.ucStringNumKeypad_out[ucCursorPos]==' ')
								   ){
									// Sposto il cursore
									hw.ucStringNumKeypad_out[--ucCursorPos]=' ';
								}
								// sono sull'ultimo carattere che � non blank, lo sbianco senza spostare
								// il cursore
								else{
									// Sposto il cursore
									hw.ucStringNumKeypad_out[ucCursorPos]=' ';
								}
								defSetWinParam(NumK_CursorPos,ucCursorPos);
							}
							break;
						case enumBtnNumK_goto_t9:
							// abilitazione t9... per provare, sempre abilitato
							// paramsNumK.ucEnableT9=1;
							if (    paramsNumK.ucT9_enabled
								 && paramsNumK.ucT9_candidate
								 &&(paramsNumK.ucT9_candidate!=0xFF)
								 &&(ucCursorPos<paramsNumK.ucMaxNumChar)
							   ){
								hw.ucStringNumKeypad_out[ucCursorPos]=paramsNumK.ucT9_candidate;
								paramsNumK.ucT9_candidate=0xFF;
								paramsNumK.ucT9_lastButton=0xFF;
								if (  (ucCursorPos<paramsNumK.ucMaxNumChar-1)
									&&(ucCursorPos<strlen(hw.ucStringNumKeypad_out))
								   ){
									// Sposto il cursore
									++ucCursorPos;
									defSetWinParam(NumK_CursorPos,ucCursorPos);
								}
							}

							paramsNumK.ucT9_found=0;
							paramsNumK.ucT9_numLoop=0;
							paramsNumK.ucT9_lastCharIdx=0;
							paramsNumK.ucT9_lastButton=0xFF;
							paramsNumK.ucT9_pressedButton=0;
							paramsNumK.ucT9_candidate=0xFF;
							if (paramsNumK.ucEnableT9){
								if (!paramsNumK.ucT9_enabled){
									paramsNumK.ucT9_enabled=1;
									ucLcdRefreshTimeoutEnable(700);
								}
								else {
									paramsNumK.ucT9_enabled=0;
									ucLcdRefreshTimeoutEnable(uiSaveTimeoutLcd_NumK);
								}
							}
							break;
						case enumBtnNumK_Dot:
							if (    paramsNumK.ucT9_enabled
								 && paramsNumK.ucT9_candidate
								 &&(paramsNumK.ucT9_candidate!=0xFF)
								 &&(ucCursorPos<paramsNumK.ucMaxNumChar)
							   ){
								hw.ucStringNumKeypad_out[ucCursorPos]=paramsNumK.ucT9_candidate;
								paramsNumK.ucT9_candidate=0xFF;
								paramsNumK.ucT9_lastButton=0xFF;
							}
							// se � il primo tasto che premiamo... la stringa si deve resettare
							if (defGetWinParam(NumK_JustEntered)==1){
								// reset stringa_out
								vStr_fillBlank(hw.ucStringNumKeypad_out,paramsNumK.ucMaxNumChar);
								// posizione cursore=0
								defSetWinParam(NumK_CursorPos,0);
							}
							// posso qui inserire il decimal point???
							else if (!ucPictureNumK_DecimalPointOk(ucCursorPos))
								continue;
							if (ucCursorPos<paramsNumK.ucMaxNumChar){
								hw.ucStringNumKeypad_out[ucCursorPos]='.';
							}
							// sposto cursore se possibile
							if (ucCursorPos<paramsNumK.ucMaxNumChar-1){
								// Sposto il cursore
								defSetWinParam(NumK_CursorPos,++ucCursorPos);
							}
							break;
						case enumBtnNumK_TheNumber:
							// reset stringa_out
							vStr_fillBlank(hw.ucStringNumKeypad_out,paramsNumK.ucMaxNumChar);
							// posizione cursore=0
							defSetWinParam(NumK_CursorPos,0);
							paramsNumK.ucT9_found=0;
							paramsNumK.ucT9_numLoop=0;
							paramsNumK.ucT9_lastCharIdx=0;
							paramsNumK.ucT9_lastButton=0xFF;
							paramsNumK.ucT9_pressedButton=0;
							paramsNumK.ucT9_candidate=0xFF;

							break;
					}
					// dato che � stato premuto almeno un tasto, reset del flag che indica 
					// che siamo appena entrati nella win
					defSetWinParam(NumK_JustEntered,0);
				}
				i++;
			}

			if (paramsNumK.ucT9_enabled){
				if (!paramsNumK.ucT9_pressedButton){
					if (paramsNumK.ucT9_lastButton!=0xFF){
						if (++paramsNumK.ucT9_numLoop>=defMinLoopT9_ValidChar){
							// inserisci testo ultimo selezionato
							paramsNumK.ucT9_found=ucCodes_T9[paramsNumK.ucT9_lastButton].ucChars[paramsNumK.ucT9_lastCharIdx];
							paramsNumK.ucT9_numLoop=0;
							paramsNumK.ucT9_lastCharIdx=0;
							paramsNumK.ucT9_candidate=0xFF;
							paramsNumK.ucT9_lastButton=0xFF;
						}
					}
					else
						paramsNumK.ucT9_numLoop=0;

				}
				else{
					paramsNumK.ucT9_pressedButton=0;
				}
				if (paramsNumK.ucT9_found){
					// se � il primo tasto che premiamo... la stringa si deve resettare
					if (defGetWinParam(NumK_JustEntered)==1){
						// reset stringa_out
						vStr_fillBlank(hw.ucStringNumKeypad_out,paramsNumK.ucMaxNumChar);
						// posizione cursore=0
						defSetWinParam(NumK_CursorPos,0);
					}
					// per prudenza, se sono entro la parte lecita, assegno il carattere
					if (ucCursorPos<paramsNumK.ucMaxNumChar){
						hw.ucStringNumKeypad_out[ucCursorPos]=paramsNumK.ucT9_found;
					}
					// sposto cursore se possibile
					if (ucCursorPos<paramsNumK.ucMaxNumChar-1){
						// Sposto il cursore
						defSetWinParam(NumK_CursorPos,++ucCursorPos);
					}
					paramsNumK.ucT9_found=0;
				}
			}



			// print di tutti i buttons della win, tranne title e numero inserito e unit� di misura...
			i=0;
			while(i<enumBtnNumK_NumOf_normal-2){
				//strcpy(hw.ucString,numk_buttons[i].stringa);
				if (paramsNumK.ucT9_enabled){
					ucPrintBitmapButton(numk_buttons_T9[i].bitmap_type,numk_buttons_T9[i].row,numk_buttons_T9[i].col,i);
				}				
				else{
					if (!paramsNumK.ucEnableT9&&(i==enumBtnNumK_goto_t9)){
                    }
                    else{
                        ucPrintBitmapButton(numk_buttons[i].bitmap_type,numk_buttons[i].row,numk_buttons[i].col,i);
                    }
				}
				i++;
			}
			if (uc_use_hexadecimal_letters){
				for (i=0;i<6;i++){
					char cstring[12];
					cstring[0]=' ';
					cstring[1]='A'+i;
					cstring[2]=' ';
					cstring[3]=0;
					ucPrintStaticButton(cstring
								,defRowStartNumKeypad+4*defOffsetYButtonsNumKeypad-i*defOffsetYButtonsNumKeypad+4
								,4*defOffsetXButtonsNumKeypad+defColStartNumKeypad 
								,enumFontMedium,
								enumBtnNumK_A+i,
								defLCD_Color_Trasparente
								);
				}
			}

			if ((paramsNumK.enumUm>enumUm_none)&&(paramsNumK.enumUm<enumUm_NumOf)){

				//unsigned char *pucStringLang(enumStringhe20char stringa){
				
				if ((paramsNumK.enumUm==enumUm_password)||(paramsNumK.enumUm==enumUm_modify_password)){
					sprintf(hw.ucString,"%s",paramsNumK.ucTitle);
				}
				else{
					sprintf(hw.ucString,"%s [%s]",paramsNumK.ucTitle,pucStringLang(enumStr20_FirstUm+paramsNumK.enumUm));
				}
			}
			else{
				mystrcpy(hw.ucString,paramsNumK.ucTitle,sizeof(hw.ucString)-1);
			}
			if (strlen(hw.ucString)<=20)
				ucPrintTitleButton(hw.ucString,defNumK_TitleRow,defNumK_TitleCol,enumFontMedium,enumBtnNumK_Title,defLCD_Color_Trasparente,1);
			else
				ucPrintTitleButton(hw.ucString,defNumK_TitleRow,defNumK_TitleCol,enumFontSmall,enumBtnNumK_Title,defLCD_Color_Trasparente,1);

			if (strlen(hw.ucStringNumKeypad_out)<=defLcdWidthX_pixel/defFontPixelSizeFromFontType(enumFontMedium)){
				fontButton=enumFontMedium;
			}
			else{
				fontButton=enumFontSmall;
			}
			uiColStartNumKeypad_value=(defLcdWidthX_pixel-strlen(hw.ucStringNumKeypad_out)*defFontPixelSizeFromFontType(fontButton))/2;

			if (paramsNumK.ucT9_enabled){
				if (paramsNumK.ucT9_candidate&&(paramsNumK.ucT9_candidate!=0xFF)){
					uiColonna=ucFontSizePixelXY[fontButton][0];
					uiColonna*=ucCursorPos;
					uiColonna+=uiColStartNumKeypad_value;
					// stampo a video il cursore...
					ucPrintCursorText(  paramsNumK.ucT9_candidate,
										defRowStartNumKeypad_value, 
										uiColonna,
										fontButton
									  );
				}
			}
			// sto inserendo la password????
			if (paramsNumK.enumUm==enumUm_password){
				// stampo a video il cursore...
				// stampo il numero correntemente inserito?? no! stampo tutti asterischi!
				i=0;
				while (i<strlen(hw.ucStringNumKeypad_out)){
					if (hw.ucStringNumKeypad_out[i]==' ')
						hw.ucString[i]=' ';
					else
						hw.ucString[i]='*';
					i++;
				}
				hw.ucString[i]=0;
				ucPrintCursorText(  hw.ucString[ucCursorPos],
									defRowStartNumKeypad_value, 
									uiColStartNumKeypad_value+ucCursorPos*ucFontSizePixelXY[fontButton][0],
									fontButton
								  );
				ucPrintStaticButton(hw.ucString,
									defRowStartNumKeypad_value,
									uiColStartNumKeypad_value,
									fontButton,
									enumBtnNumK_TheNumber,
									defLCD_Color_Trasparente
									);
			}
			else{
				uiColonna=ucFontSizePixelXY[fontButton][0];
				uiColonna*=ucCursorPos;
				uiColonna+=uiColStartNumKeypad_value;
				// stampo a video il cursore...
				ucPrintCursorText(  hw.ucStringNumKeypad_out[ucCursorPos],
									defRowStartNumKeypad_value, 
									uiColonna,
									fontButton
								  );
				// stampo il numero correntemente inserito
				ucPrintStaticButton(hw.ucStringNumKeypad_out,
									defRowStartNumKeypad_value,
									uiColStartNumKeypad_value,
									fontButton,
									enumBtnNumK_TheNumber,
									defLCD_Color_Trasparente
									);
			}
			return 1;
	}//switch(uiGetStatusCurrentWindow())
}//unsigned char ucHW_Num_Keypad(void)


// variabili con le info sul codice prg corrente
//xdata at 0x2D14 TipoStructHandleInserimentoCodice his;
xdata TipoStructHandleInserimentoCodice his;
xdata TipoStructHandleInserimentoCodice his_privato;


// restituisce 1 se la struttura his contiene un codice prodotto valido...
unsigned char ucIsValidHis(TipoStructHandleInserimentoCodice *phis){
	unsigned char xdata *xdata pc;
	unsigned int xdata i;
	unsigned int xdata uiCrc;
	if (his.uiValidKey_0xa536!=0xa536)
		return 0;
	pc =(unsigned char xdata *)phis;
	uiCrc=0;
	i=0;
	while (i<sizeof(*phis)-sizeof(phis->uiCrc)){
		uiCrc^=*pc++;
		i++;
	}
	uiCrc=~uiCrc;
	if (uiCrc!=phis->uiCrc){
		return 0;
	}
	return 1;
}//unsigned char ucIsValidHis(void)

// rende valida la struttura his, impostando crc e validkey, per avere un qlcs che ci permetta di dire
// se la struttura � valida o no...
void vValidateHis(TipoStructHandleInserimentoCodice *phis){
	unsigned char xdata *xdata pc;
	unsigned int xdata i;
	unsigned int xdata uiCrc;
	phis->uiValidKey_0xa536=0xa536;
	pc =(unsigned char xdata *)phis;
	uiCrc=0;
	i=0;
	while (i<sizeof(phis)-sizeof(phis->uiCrc)){
		uiCrc^=*pc++;
		i++;
	}
	uiCrc=~uiCrc;
	phis->uiCrc=uiCrc;
}//void vValidateHis(void)


code TipoStructInfoFieldCodice_Lingua ifc[5]={
	{0,5, "NNNNN","%-5.3f",MIN_DIAM_FILO,MAX_DIAM_FILO,&his_privato.ucDiametroFilo[0],    1,1,enumStr20_diametro_filo,enumUm_mm,".nnnn","%-5.4f"},
	{6,4, "xxxx", "%4s",   0,            0,            &his_privato.ucTipoFilo[0],        0,0,enumStr20_tipo_filo,enumUm_none},
	{11,4,"NNNN", "%-4.2f",MIN_DIAM_MD,  MAX_DIAM_MD,  &his_privato.ucDiametroMandrino[0],1,1,enumStr20_diametro_mandrino,enumUm_mm,".nnn","%-4.3f"},
	{16,2,"nn",   "%-2s",   MIN_NUM_FILI, MAX_NUM_FILI, &his_privato.ucNumFili[0],         1,1,enumStr20_numero_di_fili,enumUm_num},
	{18,2,"nn",   "%2s",   MIN_FORNI,    MAX_FORNI,    &his_privato.ucFornitore[0],       1,1,enumStr20_tolleranza,enumUm_num}
};

// permette di trovare ed impostare il numero del codice prodotto attivo
void  vSetActiveNumberProductCode(TipoStructHandleInserimentoCodice *phis){
	// indico di effettuare la ricerca esatta
	mystrcpy(scp.ucString,phis->ucCodice,sizeof(scp.ucString)-1);
	scp.ucFindExactCode=1;
	if (!hprg.firstPrg)
		scp.uiStartRecord=0;
	else
		scp.uiStartRecord=hprg.firstPrg;
	scp.uiNumMaxRecord2Find=1;
	// se il codice esiste...
	if (ucSearchCodiceProgramma()){
		// solo se prg not running vado ad impostare il programma running!!!!!!
		if (!PrgRunning){
			// uiSetRunningPrg(scp.uiLastRecordFound);
		}
		uiSetActPrg(scp.uiLastRecordFound);
	}
}

// inizializza i campi diametro filo, tipo filo, diametro mandrino etc della struttura his
void vInitializeHisFields(TipoStructHandleInserimentoCodice *phis){
	xdata unsigned char i;
	// travaso i dati dalla variabile di ingresso alla variabile his_privato
	memcpy(&his_privato,phis,sizeof(*phis));
	i=0;
	while (i<sizeof(ifc)/sizeof(ifc[0])){
		strncpy(ifc[i].pc,&phis->ucCodice[ifc[i].ucStartField],ifc[i].ucLenField);
		ifc[i].pc[ifc[i].ucLenField]=0;
		i++;
	}
	// travaso i dati dalla variabile his_privato alla variabile richiesta
	memcpy(phis,&his_privato,sizeof(*phis));
}

void vBuildHisCodeFromFields(TipoStructHandleInserimentoCodice *phis){
	// compongo il codice prodotto corrente...
	sprintf(phis->ucCodice,"%-5.5s %-4.4s %-4.4s %-2s%-2s", 
					phis->ucDiametroFilo,
					phis->ucTipoFilo,
					phis->ucDiametroMandrino,
					phis->ucNumFili,
					phis->ucFornitore
		   );

}


// finestra di gestione della selezione/inserimento codice prodotto
unsigned char ucHW_codiceProdotto(void){
	// parametro che al ritorno dal numeric keypad indica qual � il parametro che � stato modificato
	#define Win_CP_field 0
	#define Win_CP_doSearch 1

	// riga e colonna del button che contiene il codice...
	#define defBCP_Codice_row 52
	#define defBCP_Codice_col 0

	#define defBCP_Codice_rowTitle (8)
	#define defBCP_Codice_colTitle (32)
	#define defBCP_Codice_rowCodes (defBCP_Codice_row+36)

	#define defOffsetBetweenRowCodesBCP 28


	#define defBCP_Codice_rowAltri (defLcdWidthY_pixel-24-20-32)
	#define defBCP_Codice_colAltri (defLcdWidthX_pixel-2-40)

	#define defBCP_Codice_rowDelete (defLcdWidthY_pixel-24-20)
	#define defBCP_Codice_colDelete (defLcdWidthX_pixel-14*8-12)


	#define defBCP_Codice_rowClr (defBCP_Codice_rowDelete)
	#define defBCP_Codice_colClr 4

	#define defBCP_Codice_rowDistanziatore defBCP_Codice_rowClr
	#define defBCP_Codice_colDistanziatore (defLcdWidthX_pixel-10*8-12)

	#define defBCP_Codice_rowTempComp defBCP_Codice_rowClr
	#define defBCP_Codice_colTempComp (defBCP_Codice_colDistanziatore-10*8-12)

	#define defBCP_Codice_rowView (defBCP_Codice_rowDelete)
	#define defBCP_Codice_colView  (defLcdWidthX_pixel/2-24)


	// pulsanti della finestra
	typedef enum {
			enumBCP_codice=0,
			enumBCP_codice_1,
			enumBCP_codice_2,
			enumBCP_codice_3,
			enumBCP_codice_4,


			enumBCP_Clr,
			enumBCP_DeleteCodice,
			enumBCP_ViewCodice,
			enumBCP_Distanziatore,
			enumBCP_TempComp,
			enumBCP_Title,
			enumBCP_Altri_Up,
			enumBCP_Altri_Dn,

			enumBCP_Sx,
			enumBCP_Up,
			enumBCP_Cut,
			enumBCP_Esc,
			enumBCP_Dn,
			enumBCP_Dx,


			enumBCP_NumOfButtons
		}enumButtons_CodiceProdotto;

	typedef enum {
			enumField_Codice_diam_filo=0,
			enumField_Codice_tipo,
			enumField_Codice_diam_mand,
			enumField_Codice_num_fili,
			enumField_Codice_fornitore,
			enumField_Codice_NumOf
		}enumField_Codice_Field;

	xdata unsigned char i,ucField,ucMaskFieldsEnabled,ucResult;
	xdata static unsigned char ucTheCodeIsvalid,ucVisualizeUpArrow,ucMaskLastSearch,ucEnableDistanziatore,ucEnableTempComp;
	xdata float fAux[2];
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			// se ritorno da finestra info...non faccio nulla
			if (defGetWinParam(Win_CP_field)==201){
			}
			// se ritorno da inserimento parametri compensazione temperatura
			else if (defGetWinParam(Win_CP_field)==133){
				if (defGetWinParam(defIdxParam_ArchivioProdotti_showDeleteButton)==0)
					ucReturnFromWindow();
				else
					vJumpToWindow(enumWinId_MainMenu);
				return 2;
			}
			// se richiesta visualizzazione codice...
			else if (defGetWinParam(Win_CP_field)==101){
				// refresh completo della finestra...
				vSetStatusCurrentWindow(enumWindowStatus_Initialize);
				return 2;
			}
			// se richiesta cancellazione codice...
			else if (defGetWinParam(Win_CP_field)==100){
				if (uiGetWindowParam(enumWinId_YesNo,defWinParam_YESNO_Answer)){
					DeleteProgramma(uiGetActPrg());
					// reset delle chiavi di ricerca...
					//sprintf(his_privato.ucDiametroFilo,ifc[enumField_Codice_diam_filo].ucFmtField,ifc[enumField_Codice_diam_filo].fMin);
					//sprintf(his_privato.ucDiametroMandrino,ifc[enumField_Codice_diam_mand].ucFmtField,ifc[enumField_Codice_diam_mand].fMin);
					sprintf(his_privato.ucDiametroFilo,ifc[enumField_Codice_diam_filo].ucFmtField,0.0);
					sprintf(his_privato.ucDiametroMandrino,ifc[enumField_Codice_diam_mand].ucFmtField,0.0);
					sprintf(his_privato.ucNumFili,ifc[enumField_Codice_num_fili].ucFmtField,"1");
					scp.uiStartRecord=hprg.firstPrg;
					defSetWinParam(Win_CP_doSearch,1);
					// inizializzo abilitazione distanziatore...
					ucEnableDistanziatore=0;
					ucEnableTempComp=0;
					macroDisableStringCodiceProdotto_Distanziatore(his_privato.ucCodice);
				}
			}
			else{
				if (defGetWinParam(Win_CP_field)==enumField_Codice_fornitore){			
					// se ritorna 0xff, non � stata fatta nessuna selezione...
					if (uiGetWindowParam(enumWinId_Fornitori,defIdxParam_enumWinId_Fornitori_Choice)==0xFF){
						vSetStatusCurrentWindow(enumWindowStatus_Active);
						return 2;
					}
					// copio il valore inserito tramite numeric keypad nel campo di pertinenza
					// l'indice del parametro � stato salvato in Win_CP_field
					sprintf(ifc[enumField_Codice_fornitore].pc,"%-*i",ifc[enumField_Codice_fornitore].ucLenField,uiGetWindowParam(enumWinId_Fornitori,defIdxParam_enumWinId_Fornitori_Choice));
				}
				else{
					// copio il valore inserito tramite numeric keypad nel campo di pertinenza
					// l'indice del parametro � stato salvato in Win_CP_field
					sprintf(ifc[defGetWinParam(Win_CP_field)].pc,"%-*s",paramsNumK.ucMaxNumChar,hw.ucStringNumKeypad_out);
					// metto il tappo per sicurezza...
					ifc[defGetWinParam(Win_CP_field)].pc[paramsNumK.ucMaxNumChar]=0;
				}
				defSetWinParam(Win_CP_doSearch,1);
				if (!hprg.firstPrg)
					scp.uiStartRecord=0;
				else
					scp.uiStartRecord=hprg.firstPrg;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			// reset struttura di ricerca codice prodotto
			memset(&scp,0,sizeof(scp));
			ucTheCodeIsvalid=0;
			if (!hprg.firstPrg)
				scp.uiStartRecord=0;
			else
				scp.uiStartRecord=hprg.firstPrg;
			// indico di effettuare la ricerca tramite codici jolly
			scp.ucFindExactCode=0;
            i=0;
			while(i<enumBCP_NumOfButtons){
				ucCreateTheButton(i); 
                i++;
            }
			// stringa completamente vuota per visualizzare tutti i codici
			//strcpy(his_privato.ucCodiceIngresso,his_privato.ucCodice);
			memset(his_privato.ucCodiceIngresso,' ',sizeof(his_privato.ucCodiceIngresso)-1);
			his_privato.ucCodiceIngresso[sizeof(his_privato.ucCodiceIngresso)-1]=0;
			// inizializzo i vari campi...(diametro mandrino, diametro filo etc)
            i=0;
			while (i<sizeof(ifc)/sizeof(ifc[0])){
				strncpy(ifc[i].pc,&his_privato.ucCodice[ifc[i].ucStartField],ifc[i].ucLenField);
				ifc[i].pc[ifc[i].ucLenField]=0;
                i++;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			his_privato.button.ucType=enumButton_Field;   // tipo del bottone che deve contornare il testo; 0=nessuno
			his_privato.button.ucNum=enumBCP_codice;    // numero del bottone associato al testo
			his_privato.button.ucNumOfButtonFields=5;
			ucMaskLastSearch=0;
            i=0;
			// copio le strutture con info sui vari fields...
			while(i<5){
				his_privato.button.ucCharStartField[i]=ifc[i].ucStartField;
				his_privato.button.ucLenField[i]=ifc[i].ucLenField;
                i++;
			}
			// parto con codice=blank in modo da listare tutti i codici esistenti
			// reset delle chiavi di ricerca...
			sprintf(his_privato.ucDiametroFilo,ifc[enumField_Codice_diam_filo].ucFmtField,0.0);
			sprintf(his_privato.ucDiametroMandrino,ifc[enumField_Codice_diam_mand].ucFmtField,0.0);
			// versione 1.42--> imposto per default numero fili=1
			sprintf(his_privato.ucNumFili,ifc[enumField_Codice_num_fili].ucFmtField,"1");
			scp.uiStartRecord=hprg.firstPrg;
			// inizializzo abilitazione distanziatore...
			ucEnableDistanziatore=macroIsStringaCodiceProdotto_Distanziatore(his_privato.ucCodice);
			// inizializzo abilitazione compensazione temperatura...
			ucEnableTempComp=macroIsStringaCodiceProdotto_TemperatureCompensated(his_privato.ucCodice);
			// impostazione variabili locali della finestra...
			// indico di fare una ricerca per riempire i campi
			defSetWinParam(Win_CP_doSearch,1);
			ucVisualizeUpArrow=0;
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
            i=0;
			while(i<enumBCP_NumOfButtons){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						case enumBCP_DeleteCodice:
							// se il codice in ingresso � quello "ufficiale"
							// e programma in corso oppure ci sono comunque lavori in lista, non posso cancellare
							if (  (PrgRunning||(pJobsSelected_Jobs->ucNumLavoriInLista||spiralatrice.firstComm))
								&&(memcmp(his_privato.ucCodiceIngresso,his_privato.ucCodice,sizeof(his_privato.ucCodiceIngresso))==0)
								){
								// visualizzo la finestra: password non corretta
								// numero msg da visualizzare: 3
								ucSetWindowParam(enumWinId_Info,0, 3);
								// messaggi da visualizzare
								ucSetWindowParam(enumWinId_Info,1, enumStr20_cannot_delete);
								ucSetWindowParam(enumWinId_Info,2, enumStr20_prgrunning);
								ucSetWindowParam(enumWinId_Info,3, enumStr20_jobinlist);
								// messaggio: password KO
								defSetWinParam(Win_CP_field,201);
								// chiamo la finestra info
								ucCallWindow(enumWinId_Info);
								return 2;
							}
							// indico di effettuare la ricerca esatta
							mystrcpy(scp.ucString,his_privato.ucCodice,sizeof(scp.ucString)-1);
							scp.ucFindExactCode=1;
							if (!hprg.firstPrg)
								scp.uiStartRecord=0;
							else
								scp.uiStartRecord=hprg.firstPrg;
							scp.uiNumMaxRecord2Find=1;
							// se il codice esiste...
							if (ucSearchCodiceProgramma()){
								uiSetActPrg(scp.uiLastRecordFound);
								// chiedo conferma
								vStringLangCopy(hw.ucStringNumKeypad_in,enumStr20_DeleteCode);
								mystrcpy(hw.ucStringNumKeypad_out,his_privato.ucCodice,sizeof(hw.ucStringNumKeypad_out)-1);
								defSetWinParam(Win_CP_field,100);
								// impostazione del parametro 0 al valore 0--> visualizza pulsanti yes e no
								ucSetWindowParam(enumWinId_YesNo,def_WinYesNo_Param_TipoPulsante, def_WinYesNo_PUlsantiYesNo);
								// visualizzo yes/no...
								ucCallWindow(enumWinId_YesNo);
								return 2;
							}
							else
								break;
						case enumBCP_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;

						case enumBCP_Title:
						case enumBCP_Esc:
							// recupero il codice con cui ero entrato nella finestra??
							// no, lo resetto per indicare che sono uscito con esc
							memset(&his_privato,0,sizeof(his_privato));

							// chiamo il main menu...
							vJumpToWindow(enumWinId_MainMenu);
							// indico di rinfrescare la finestra
							return 2;

						// questo button � diviso in fields!
						case enumBCP_codice:
							ucField=ucFieldButtonPressed(i);
							// indico qual � il parametro che sto inserendo...
							defSetWinParam(Win_CP_field,ucField);
							// controllo per sicurezza che il codice sia uno di quelli gestiti...
							if (ucField>=enumField_Codice_NumOf)
								break;
							// se il campo � quello della tolleranza filo, faccio una call alla finestra di selezione...
							if (ucField==enumField_Codice_fornitore){
								// indico che la tabella NON pu� essere modificata
								ucSetWindowParam(enumWinId_Fornitori,defIdxParam_enumWinId_Fornitori_ModifyEnable,0);
								// chiamo la finestra fornitori
								ucCallWindow(enumWinId_Fornitori);
								return 2;
							}

							if (ucField==enumField_Codice_tipo)
								paramsNumK.ucEnableT9=1;
							else
								paramsNumK.ucEnableT9=0;

							paramsNumK.enumUm=ifc[ucField].enumUm; // unit� di misura...
							// imposto numero max di caratteri
							paramsNumK.ucMaxNumChar=ifc[ucField].ucLenField;	
							// imposto limiti min e max
							paramsNumK.fMin=ifc[ucField].fMin;
							// il numero massimo di fili � un parametro macchina...
							if (ucField==enumField_Codice_num_fili)
								paramsNumK.fMax=macParms.ucMaxNumFili;
							else
								paramsNumK.fMax=ifc[ucField].fMax;

							if ((nvram_struct.actUM==UM_IN)&&(ucIsUmSensitive(ifc[ucField].enumUm))){
								paramsNumK.fMin*=MM_TO_INCH;
								paramsNumK.fMax*=MM_TO_INCH;
								paramsNumK.enumUm++;
								// imposto stringa di picture
								mystrcpy((char*)paramsNumK.ucPicture,(char*)ifc[ucField].ucPictFieldInch,sizeof(paramsNumK.ucPicture)-1);
							}
							else
								// imposto stringa di picture
								mystrcpy((char*)paramsNumK.ucPicture,(char*)ifc[ucField].ucPictField,sizeof(paramsNumK.ucPicture)-1);
							//strcpy(paramsNumK.ucTitle,ifc[ucField].ucTitleNumK);
							vStringLangCopy(paramsNumK.ucTitle,ifc[ucField].stringa);
							// copio la stringa con cui si inizializza il numeric keypad
							mystrcpy(hw.ucStringNumKeypad_in,ifc[ucField].pc,sizeof(hw.ucStringNumKeypad_in)-1);
							// imposto abilitazione uso stringa picture
							paramsNumK.ucPictureEnable=ifc[ucField].ucPictureEnable;	// abilitazione stringa picture
							// imposto abilitazione uso min/max
							paramsNumK.ucMinMaxEnable=ifc[ucField].ucMinMaxEnable;	// abilitazione controllo valore minimo/massimo
							// chiamo il numeric keypad 
							ucCallWindow(enumWinId_Num_Keypad);
							// indico di rinfrescare la finestra
							return 2;
						case enumBCP_Clr:
							// reset delle chiavi di ricerca...
							sprintf(his_privato.ucDiametroFilo,ifc[enumField_Codice_diam_filo].ucFmtField,0.0);
							sprintf(his_privato.ucDiametroMandrino,ifc[enumField_Codice_diam_mand].ucFmtField,0.0);
							// versione 1.42--> imposto per default numero fili=1
							sprintf(his_privato.ucNumFili,ifc[enumField_Codice_num_fili].ucFmtField,"1");
							scp.uiStartRecord=hprg.firstPrg;
							defSetWinParam(Win_CP_doSearch,1);
							// inizializzo abilitazione distanziatore...
							ucEnableDistanziatore=0;
							ucEnableTempComp=0;
							macroDisableStringCodiceProdotto_Distanziatore(his_privato.ucCodice);
							return 2;
						case enumBCP_codice_1:
						case enumBCP_codice_2:
						case enumBCP_codice_3:
						case enumBCP_codice_4:
							// copio la stringa
							mystrcpy(his_privato.ucCodice,scp.ucFoundString[i-enumBCP_codice_1],sizeof(his_privato.ucCodice)-1);
							// trovo i vari campi...
                            i=0;
							while (i<sizeof(ifc)/sizeof(ifc[0])){
								strncpy(ifc[i].pc,&his_privato.ucCodice[ifc[i].ucStartField],ifc[i].ucLenField);
								ifc[i].pc[ifc[i].ucLenField]=0;
                                i++;
							}
							// inizializzo abilitazione distanziatore...
							ucEnableDistanziatore=macroIsStringaCodiceProdotto_Distanziatore(his_privato.ucCodice);
							// inizializzo abilitazione compensazione temperatura...
							ucEnableTempComp=macroIsStringaCodiceProdotto_TemperatureCompensated(his_privato.ucCodice);

							// indico di rifare il search...
							defSetWinParam(Win_CP_doSearch,1);
							break;
						case enumBCP_Up:
							if (scp.uiNumRecordFound){
								scp.uiStartRecord=scp.uiNumRecordFoundString[0];
							}
							else{
								scp.uiStartRecord=hprg.firstPrg;
							}
							defSetWinParam(Win_CP_doSearch,1);
							scp.direction=enumDirectionBackward_SCP;
							break;
						case enumBCP_Dn:
						//case enumBCP_Altri:
							if (scp.uiNumRecordFound>3){
								ucVisualizeUpArrow=1;
								defSetWinParam(Win_CP_doSearch,1);
								scp.uiStartRecord=scp.uiNumRecordFoundString[2]; //scp.uiLastRecordFound;
								scp.direction=enumDirectionForward_SCP;
							}
							break;
						case enumBCP_Distanziatore:
							ucEnableDistanziatore=!ucEnableDistanziatore;
							break;
						case enumBCP_TempComp:
							ucEnableTempComp=!ucEnableTempComp;
							break;
						case enumBCP_Dx:
						case enumBCP_ViewCodice:
							if (!ucTheCodeIsvalid)
								break;

							vVerifyInsertCodiceProdotto(his_privato.ucCodice);

							if (i==enumBCP_ViewCodice){
								// versione 1.49: problema della visualizzazione codici prodotto da archivio codici prodotto...
								// quando si va in "visualizza codici prodotto", e poi si seleziona un codice
								// e si preme "visualizza", visualizza il codice in produzione...
								uiSetActPrg(scp.uiLastRecordFound);
								defSetWinParam(Win_CP_field,101);
								ucCallWindow(enumWinId_ViewCodiceProdotto);
							}
							else{
								// se programma in corso o esistono lotti, non posso cambiare codice prodotto!!!
								// a meno che io non abbia  modificato solo la tolleranza filo...
								// nel qual caso l'operazione � ammessa
								// quindi controllo la porzione di codice che va dall'inizio fino al numero di fili
								// escludendo la tolleranza filo...

								// siccome il codice prodotto pu� essere solo inserito, non faccio nessun controllo 
								// semplicemente inserisco
								//vJumpToWindow(enumWinId_ListaLavori);
								
								// se programma appena inserito � vuoto ed ha compensazione di temperatura abilitata, chiamo inserimento dei parametri
								if (macroIsStringaCodiceProdotto_TemperatureCompensated(his_privato.ucCodice)&&hprg.ptheAct->empty){
									defSetWinParam(Win_CP_field,133);
									ucCallWindow(enumWinId_Temperature_Compensation_Params);
								}
								else{
									if (defGetWinParam(defIdxParam_ArchivioProdotti_showDeleteButton)==0)
										ucReturnFromWindow();
									else
										vJumpToWindow(enumWinId_MainMenu);
								}
							}
							return 2;
						case enumBCP_Sx:
							// recupero il codice con cui ero entrato nella finestra
							mystrcpy(his_privato.ucCodice,his_privato.ucCodiceIngresso,sizeof(his_privato.ucCodice)-1);
							// trovo i vari campi...
                            i=0;
							while (i<sizeof(ifc)/sizeof(ifc[0])){
								strncpy(ifc[i].pc,&his_privato.ucCodice[ifc[i].ucStartField],ifc[i].ucLenField);
								ifc[i].pc[ifc[i].ucLenField]=0;
                                i++;
							}
							//vJumpToWindow(enumWinId_ListaLavori);
							// impostazione del parametro della finestra
							if (defGetWinParam(defIdxParam_ArchivioProdotti_showDeleteButton)==0)
								ucReturnFromWindow();
							else
								vJumpToWindow(enumWinId_MainMenu);
							return 2;

						default:
							break;
					}
				}
                i++;
			}
			// PRESENTAZIONE DELLA FINESTRA...

			// riformatto diametro filo e diametro mandrino
			his_privato.fAux=atof(his_privato.ucDiametroFilo);
			if (nvram_struct.actUM==UM_IN){
				sprintf(his_privato.ucDiametroFilo,ifc[enumField_Codice_diam_filo].ucFmtFieldInch,his_privato.fAux);
				// elimino lo zero iniziale, non trovo al momento altro modo!!!!!
				if (his_privato.ucDiametroFilo[0]&&(his_privato.ucDiametroFilo[0]!='.')){
					strcpy(his_privato.ucDiametroFilo,his_privato.ucDiametroFilo+1);
				}
			}
			else
				sprintf(his_privato.ucDiametroFilo,ifc[enumField_Codice_diam_filo].ucFmtField,his_privato.fAux);
			his_privato.ucDiametroFilo[defCodice_MaxCharDiametroFilo]=0;

			his_privato.fAux=atof(his_privato.ucDiametroMandrino);
			// voglio evitare overflow... siccome la precisione decimale non viene mai sacrificata dal C
			// se viene inserito 20.0 la sprintf la fa diventare 20.00 e posso corrompere la mia stringa...
			if (his_privato.fAux>=10){
				sprintf(his_privato.ucDiametroMandrino,"%-4.1f",his_privato.fAux);
			}
			else{
				if (nvram_struct.actUM==UM_IN){
					sprintf(his_privato.ucDiametroMandrino,ifc[enumField_Codice_diam_mand].ucFmtFieldInch,his_privato.fAux);
					// elimino lo zero iniziale, non trovo al momento altro modo!!!!!
					if (his_privato.ucDiametroMandrino[0]&&(his_privato.ucDiametroMandrino[0]!='.')){
						strcpy(his_privato.ucDiametroMandrino,his_privato.ucDiametroMandrino+1);
					}
				}
				else
					sprintf(his_privato.ucDiametroMandrino,ifc[enumField_Codice_diam_mand].ucFmtField,his_privato.fAux);
			}
			his_privato.ucDiametroMandrino[defCodice_MaxCharDiametroMandrino]=0;
			

			// compongo il codice prodotto corrente...
			sprintf(his_privato.ucCodice,"%s %s %s %-2s%-2s", his_privato.ucDiametroFilo,
													his_privato.ucTipoFilo,
													his_privato.ucDiametroMandrino,
													his_privato.ucNumFili,
													his_privato.ucFornitore
					);

			// valido la struttura his_privato appena composta...
			vValidateHis(&his_privato);
			if (ucEnableDistanziatore){
				macroEnableStringCodiceProdotto_Distanziatore(his_privato.ucCodice);
			}
			else{
				macroDisableStringCodiceProdotto_Distanziatore(his_privato.ucCodice);
			}
			if (ucEnableTempComp){
				macroEnableStringCodiceProdotto_TemperatureCompensated(his_privato.ucCodice);
			}
			else{
				macroDisableStringCodiceProdotto_TemperatureCompensated(his_privato.ucCodice);
			}

			// visualizzo il button col codice...
			// codice prodotto con intestazione
			// "FILO        LEGA      MANDRINO  NF  TOLL",
			vString40LangCopy(hw.ucString,enumStr40_codefields);
			ucPrintSmallText_ColNames(hw.ucString,defBCP_Codice_row-12,defBCP_Codice_col);

			fAux[0]=atof(his_privato.ucDiametroFilo);
			fAux[1]=atof(his_privato.ucDiametroMandrino);
			ucMaskFieldsEnabled=0;
			if (nvram_struct.actUM==UM_IN){
				fAux[0]*=INCH_TO_MM;
				fAux[1]*=INCH_TO_MM;
			}
			// se diametro filo � valido, lo uso come chiave di ricerca
			if ((fAux[0]>=ifc[enumField_Codice_diam_filo].fMin)&&(fAux[0]<=ifc[enumField_Codice_diam_filo].fMax)){
				ucMaskFieldsEnabled|=0x01;
			}
			// se diametro mandrino � valido, lo uso come chiave di ricerca
			if ((fAux[1]>=ifc[enumField_Codice_diam_mand].fMin)&&(fAux[1]<=ifc[enumField_Codice_diam_mand].fMax)){
				ucMaskFieldsEnabled|=0x02;
			}
			// versione 1.29:
			// inserisco anche il numero di fili, se valido
			if (((his_privato.ucNumFili[0]>='0'+MIN_NUM_FILI)&&(his_privato.ucNumFili[0]<='0'+MAX_NUM_FILI))){
				ucMaskFieldsEnabled|=0x04;
			}

			// indico di effettuare la ricerca tramite codici jolly
			scp.ucFindExactCode=0;
			if (defGetWinParam(Win_CP_doSearch)){
				defSetWinParam(Win_CP_doSearch,0);
				// salvo la maschera con cui � stata fatta l'ultima ricerca
				ucMaskLastSearch=ucMaskFieldsEnabled;
				// cerco se esistono codici programma validi...
				// compongo il codice prodotto corrente, impostando solo diametro filo e mandrino
				// a seconda dei campi validi, compongo la stringa di ricerca, inserendo blank come jolly
				switch(ucMaskFieldsEnabled&0x03){
					case 0:
						strcpy(scp.ucString,"                    ");
						break;
					case 1:
						sprintf(scp.ucString,"%s               ", his_privato.ucDiametroFilo);
						break;
					case 2:
						sprintf(scp.ucString,"           %s     ", his_privato.ucDiametroMandrino);
						break;
					case 3:
					default:
						sprintf(scp.ucString,"%s      %s     ", his_privato.ucDiametroFilo,his_privato.ucDiametroMandrino);
						break;
				}

				scp.uiNumMaxRecord2Find=4;
				ucSearchCodiceProgramma();
				// se la ricerca era all'indietro, la ripeto in avanti per riempire il mio array
				// nel modo corretto...
				if (scp.direction==enumDirectionBackward_SCP){
					if (scp.uiStartRecord!=scp.uiLastRecordFound)
						ucVisualizeUpArrow=1;
					else
						ucVisualizeUpArrow=0;
					scp.uiStartRecord=scp.uiLastRecordFound;
					scp.direction=enumDirectionForward_SCP;
					ucSearchCodiceProgramma();
				}

			}
			if (scp.uiNumRecordFound){
                i=0;
				while (i<scp.uiNumMaxRecord2Find-1){
					if (i>=scp.uiNumRecordFound)
						break;
					mystrcpy(hw.ucString,scp.ucFoundString[i],sizeof(hw.ucString)-1);
					ucPrintStaticButton(hw.ucString,defBCP_Codice_rowCodes+i*defOffsetBetweenRowCodesBCP,defBCP_Codice_col,100,enumBCP_codice_1+i,defLCD_Color_Trasparente);
                    i++;
				}
			}


			//strcpy(hw.ucString,"Lista completa");
			// visualizzo il tasto "Lista completa" solo se non sono gi� visualizzati...
			if (ucMaskFieldsEnabled||(ucMaskLastSearch!=ucMaskFieldsEnabled)){
				vStringLangCopy(hw.ucString,enumStr20_Clr);
				ucPrintStaticButton(hw.ucString,defBCP_Codice_rowClr,defBCP_Codice_colClr,enumFontSmall,enumBCP_Clr,defLCD_Color_Trasparente);
			}
			// se spaziatore abilitato, visualizzo pulsante toggle spaziatore
			// � possibile selezionare lo spaziatore solo se sto inserendo o cercando un codice prodotto
			// idem per abilitazione compensazione temperatura
			if (!defGetWinParam(defIdxParam_ArchivioProdotti_showDeleteButton)){
				if (macParms.uc_distanziatore_type){
					vStringLangCopy(hw.ucString,enumStr20_PresenzaDistanziatore);
					ucPrintStaticButton(hw.ucString,defBCP_Codice_rowDistanziatore,defBCP_Codice_colDistanziatore,enumFontSmall,enumBCP_Distanziatore,defLCD_Color_Trasparente);
				}
				if (macParms.ucAbilitaCompensazioneTemperatura){
					vStringLangCopy(hw.ucString,enumStr20_UtilizzaCompensazioneTemperatura);
					ucPrintStaticButton(hw.ucString,defBCP_Codice_rowTempComp,defBCP_Codice_colTempComp,enumFontSmall,enumBCP_TempComp,defLCD_Color_Trasparente);
				}
			}

			//strcpy(hw.ucString,"Seleziona codice");
			vStringLangCopy(hw.ucString,enumStr20_Seleziona_Codice);
			ucPrintTitleButton(hw.ucString,defBCP_Codice_rowTitle,defBCP_Codice_colTitle,100,enumBCP_Title,defLCD_Color_Trasparente,1);

			// posso dare ok solo se ho impostato tutte le grandezze
			// se diametro filo � valido, lo uso come chiave di ricerca
			ucResult=ucMaskFieldsEnabled;
			// verifico se numero fili � corretto
			fAux[0]=atof(his_privato.ucNumFili);
			if ((fAux[0]>=ifc[enumField_Codice_num_fili].fMin)&&(fAux[0]<=ifc[enumField_Codice_num_fili].fMax)){
				ucResult|=4;
			}
			fAux[0]=atof(his_privato.ucFornitore);
			if ((fAux[0]>=ifc[enumField_Codice_fornitore].fMin)&&(fAux[0]<=ifc[enumField_Codice_fornitore].fMax)){
				ucResult|=8;
			}


			if (ucResult==15){
				ucTheCodeIsvalid=1;
			}
			else
				ucTheCodeIsvalid=0;
			// se distanziatore attivato, lo evidenzio...
			if (macroIsStringaCodiceProdotto_Distanziatore(his_privato.ucCodice)){
				hw.ucString[0]=defDistanz_SpecialCharInCode;
				hw.ucString[1]=0;
				ucPrintStaticText_MediumFont_Color(hw.ucString,defBCP_Codice_row,defBCP_Codice_col+defDistanz_IdxSpecialCharInCode*16,defLCD_Color_Red,defLCD_Color_Yellow);
			}
			// se compensazione temperatura abilitata, lo evidenzio...
			if (macroIsStringaCodiceProdotto_TemperatureCompensated(his_privato.ucCodice)){
				hw.ucString[0]=defTempComp_SpecialCharInCode;
				hw.ucString[1]=0;
				ucPrintStaticText_MediumFont_Color(hw.ucString,defBCP_Codice_row,defBCP_Codice_col+defTempComp_IdxSpecialCharInCode*16,defLCD_Color_Red,defLCD_Color_Yellow);
			}

			if (ucTheCodeIsvalid){
				ucPrintFieldButton(		his_privato.ucCodice,
										defBCP_Codice_row, 
										defBCP_Codice_col, 
										enumBCP_codice,
										&his_privato.button,
										defLCD_Color_Yellow
									);

				// solo se � permesso(archivio prodotti chiamato da main) visualizzo pulsanti di cancellazione e "vedi" codice
				if (defGetWinParam(defIdxParam_ArchivioProdotti_showDeleteButton)){
					vStringLangCopy(hw.ucString,enumStr20_ViewCode);
					ucPrintStaticButton(hw.ucString,defBCP_Codice_rowView,defBCP_Codice_colView,enumFontSmall,enumBCP_ViewCodice,defLCD_Color_Trasparente);

					// solo se codice non � in lista jobs, lo posso cancellare
					if (ucIdxCodeInJobList(his_privato.ucCodice)==0xff){
						vStringLangCopy(hw.ucString,enumStr20_DeleteCode);
						ucPrintStaticButton(hw.ucString,defBCP_Codice_rowDelete,defBCP_Codice_colDelete,enumFontSmall,enumBCP_DeleteCodice,defLCD_Color_Trasparente);
					}
				}
			}
			else{
				ucPrintFieldButton(		his_privato.ucCodice,
										defBCP_Codice_row, 
										defBCP_Codice_col, 
										enumBCP_codice,
										&his_privato.button,
										defLCD_Color_Blue
									);
			}

			if (scp.uiNumRecordFound>3){
				ucPrintBitmapButton(enumBitmap_40x24_DnArrow,defBCP_Codice_rowAltri,defBCP_Codice_colAltri,enumBCP_Altri_Dn);
				
		    }
			if ( ucVisualizeUpArrow){
				ucPrintBitmapButton(enumBitmap_40x24_UpArrow,defBCP_Codice_rowAltri,defBCP_Codice_colAltri-44,enumBCP_Altri_Up);			
		    }

			vAddStandardButtonsOk(enumBCP_Sx);

			return 1;
	}
}//

void DatiInizioCommessa(void){
extern void vSaltaSuPaginaProduzione(void);

	vSaltaSuPaginaProduzione();
}


void vHandleEscFromMenu(void){
	if (!ucIsEmptyStackCaller())
		ucReturnFromWindow();
	else
		vJumpToWindow(enumWinId_MainMenu);

}
void vDoTaglio(void){
	//if (macParms.ucUsaSensoreTaglio)
	//	vAttivaOperazioneLama(enumCiclicaLama_taglia);
	/* Indico di eseguire un taglio asincrono. */
	TaglioAsincrono=1;
    if (avvStatus == AVV_IDLE)
		avvStatus = AVV_WAIT_PRE;
}


// quando un job � locked, nel senso che non pu� essere modificato???
unsigned char ucIsLockedJob(unsigned char i){
	// se programma running, e puntatore ai job attuali=puntatore al job running
	// e sono al primo record dei job, non lo posso modificare perch� � in esecuzione
	if (	PrgRunning
		&&(pJobsSelected==pJobsRunning)
		&&(i==0)
		){
		return 1;
	}
	return 0;
}





void vInitInputEdge(void){
	memset(&aie[0],0,sizeof(aie));
}

unsigned char ucDigitalInput_PositiveEdge(enumInputEdge ie){
	xdata unsigned char uc;
	//ET0=0;
		uc =aie[ie].ucPositiveEdge;
		aie[ie].ucPositiveEdge=0;
	//ET0=1;
	return uc;
}

unsigned char ucDigitalInput_AtHiLevel(enumInputEdge ie){
	return (aie[ie].ucActualValue>0)?1:0;
}

unsigned char ucDigitalInput_AtLowLevel(enumInputEdge ie){
	return (aie[ie].ucActualValue==0)?1:0;
}

unsigned char ucDigitalInput_NegativeEdge(enumInputEdge ie){
	xdata unsigned char uc;
	//ET0=0;
		uc=aie[ie].ucNegativeEdge;
		aie[ie].ucNegativeEdge=0;
	//ET0=1;
	return uc;
}


// funzione per trasferire da commessa a lista pJobsRunning_Jobs->..
void vTrasferisciDaCommesseAlistaLavori(void){
	xdata COMMESSA * xdata pcomm;
	unsigned int xdata i;
	// svuoto completamente la lista pJobsRunning_Jobs->..
	memset(pJobsRunning_Jobs,0,sizeof(*pJobsRunning_Jobs));
	// se lavoro in modo L+R, allora posso recuperare il modo salvato in precedenza
	if (macParms.modo == MODE_LENR) {
		pJobsRunning_Jobs->ucWorkingMode_0L_1LR=1;
	}
	else{
		pJobsRunning_Jobs->ucWorkingMode_0L_1LR=0;
	}
	if (!spiralatrice.firstComm){
		return;
	}
	// ricostruisco la lista pJobsRunning_Jobs->..
	pcomm=&nvram_struct.commesse[spiralatrice.firstComm-1];
	if (macParms.modo == MODE_LENR) {
		pJobsRunning_Jobs->ucWorkingMode_0L_1LR=pcomm->ucWorkingMode_0L_1LR?1:0;
	}
	i=0;
	while (i<defMaxLavoriInLista){
		if (pcomm->uiValidComm_0xa371!=0xa371){
			pJobsRunning_Jobs->ucNumLavoriInLista=i;
			break;
		}
		pJobsRunning_Jobs->lista[i].uiNumeroPezzi=pcomm->n;
		pJobsRunning_Jobs->lista[i].fOhm=pcomm->res;
		mystrcpy(pJobsRunning_Jobs->lista[i].ucDescrizione,pcomm->commessa,sizeof(pJobsRunning_Jobs->lista[0].ucDescrizione)-1);
		pJobsRunning_Jobs->lista[i].npezzifatti=pcomm->ndo;
		pJobsRunning_Jobs->lista[i].magg=pcomm->magg;        /* Magg. pezzi da produrre. */
		pJobsRunning_Jobs->lista[i].ucValidKey=defLavoroValidoKey;
		vCodelistTouched();
		pJobsRunning_Jobs->lista[i].ucPosition=i+1;
		// imposto il modo di funzionamento...
		pJobsRunning_Jobs->ucWorkingMode_0L_1LR=pcomm->ucWorkingMode_0L_1LR;
		// se non c'� un'ulteriore commessa, esco dalla funzione
		// impostando il numero corretto di lavori in lista
		if (!pcomm->nextComm){
			pJobsRunning_Jobs->ucNumLavoriInLista=i+1;		
			break;
		}
		pcomm=&nvram_struct.commesse[pcomm->nextComm-1];
		i++;
	}
	
}

void vTrasferisciDaListaLavoriAcommesse(void){
	xdata unsigned char i;
	xdata COMMESSA * xdata pcomm;
	pcomm=&nvram_struct.commesse[0];
	// se non c'� un prg in esecuzione, la mia lista job diventa la mia lista lavori da eseguire,
	// tutto ok
	// altrimenti, posso trasferire solo se:
	// - il job da trasferire � lo stesso che � running
	// - e solo i record che non sono locked
	// quindi non ne trasferisco nessuno se prgrunning e job attuale != job running!
	if (PrgRunning&&pJobsSelected!=pJobsRunning){
		return;
	}
	// se programma in esecuzione, il puntatore deve partire dalla commessa corrente
	// per aggiornare solo i record successivi ad essa
	// l'allineamento deve essere effettuato anche nel caso in cui si passi 
	// da una commessa alla successiva automaticamente
	if (PrgRunning||NextPrgAutomatic){
		pcomm=&nvram_struct.commesse[spiralatrice.runningCommessa];
	}
	i=0;
	while (i<pJobsSelected_Jobs->ucNumLavoriInLista){
		// solo se job non � locked, posso fare il trasferimento!!!!!!
		if (!ucIsLockedJob(i)){
			pcomm->n=pJobsSelected_Jobs->lista[i].uiNumeroPezzi;
			pcomm->res=pJobsSelected_Jobs->lista[i].fOhm;
			mystrcpy(pcomm->commessa,pJobsSelected_Jobs->lista[i].ucDescrizione,sizeof(pcomm->commessa)-1);
			pcomm->numcommessa=i;
			// imposto il modo di funzionamento...
			pcomm->ucWorkingMode_0L_1LR=pJobsSelected_Jobs->ucWorkingMode_0L_1LR;
			pcomm->numprogramma=uiGetActPrg();
			pcomm->ndo=pJobsSelected_Jobs->lista[i].npezzifatti;
			// Passaggio automatico alla commessa successiva--> � un parametro macchina
			pcomm->automatico=macParms.ucEnableCommesseAutomatiche; 
			pcomm->magg=pJobsSelected_Jobs->lista[i].magg;        /* Magg. pezzi da produrre. */

			pcomm->rpm=0;
			pcomm->assorb_frizioni[0]=1.0;
			pcomm->assorb_frizioni[1]=1.0;
			pcomm->uiValidComm_0xa371=0xa371;

		}
		// versione 1.52: devo tener conto del fatto che la prima commessa da cui sono partito potrebbe non essere
		// la zero, per cui � meglio utilizzare il puntatore corrente per definire l'indice...
		// se sono all'ultima entry, nextcomm deve valere zero!
		if (i>=pJobsSelected_Jobs->ucNumLavoriInLista-1)
			pcomm->nextComm=0;
		// il puntatore alla prossima commessa lo trovo impostando (indice attuale+1)+1 (perch� l'indice commessa � 1-based)
		// quindi praticamente indice attuale +2 
		else
			pcomm->nextComm=2+(pcomm-&nvram_struct.commesse[0]);
		// se sono alla prima entry, non ho nessuna commessa prima di me!
		if (i==0)
			pcomm->prevComm=0;
		// se sono su una entry successiva alla prima, la commessa precedente � data da:
		// (<indice attuale> -1)+1 (perch� l'indice commessa � 1-based)
		// per cui alla fine uso <indice attuale> per puntare alla commessa precedente
		else
			pcomm->prevComm=(pcomm-&nvram_struct.commesse[0]);
		pcomm++;
		i++;
	}
	if (!PrgRunning&&!NextPrgAutomatic){
		// imposto indice alla prima commessa
		if (pJobsSelected_Jobs->ucNumLavoriInLista)
			spiralatrice.firstComm=1;
		else
			spiralatrice.firstComm=0;
		// imposto puntatore alla commessa corrente=prima commessa in lista...
		if (spiralatrice.firstComm){
			spiralatrice.actCommessa=spiralatrice.firstComm-1;
			spiralatrice.runningCommessa=spiralatrice.actCommessa;
		}
	}
}//void vTrasferisciDaListaLavoriAcommesse(void)



code unsigned char uc_StartCopy[]={0,6,11,16,18};
code unsigned char uc_LengthCopy[]={5,4,4,1,1};

// finestra di gestione della selezione/inserimento lista pJobsSelected_Jobs->..
unsigned char ucHW_listaLavori(void){
	// parametro che al ritorno dal numeric keypad indica qual � il parametro che � stato modificato
	#define Win_CP_field 0
	#define Win_CP_doSearch 1


	#define defOffsetRow_listaLavori 46
	#define defLL_Codice_rowTitle (8)
	#define defLL_Codice_colTitle (8)
	// riga e colonna del button che contiene il codice...
	#define defLL_Codice_row 44
	#define defLL_Codice_col 0
	#define defLL_Codice_rowUp (defLL_Codice_row)
	#define defLL_Codice_rowNameCols (defLL_Codice_rowUp+28)
	#define defLL_Codice_rowCodes (defLL_Codice_rowNameCols+14)
	#define defLL_Codice_rowDn (defLL_Codice_rowCodes+defNumLavoriDaVisualizzare*42-4)

	#define defLL_Codice_rowOk (defLcdWidthY_pixel-32-8)
	#define defLL_Codice_colOk (defLcdWidthX_pixel-32*2-8)

	#define defLL_Codice_rowPrevious (defLL_Codice_rowOk)
	#define defLL_Codice_colPrevious (defLL_Codice_colOk-32*2-8)


	// pulsanti della finestra
	typedef enum {
			enumLL_codice=0,
			enumLL_lavoro_1,
			enumLL_lavoro_2,
			enumLL_lavoro_3,
			enumLL_lavoro_4,
			enumLL_desclavoro_1,
			enumLL_desclavoro_2,
			enumLL_desclavoro_3,
			enumLL_desclavoro_4,
			enumLL_SingleJob,
			enumLL_Mode,
			enumLL_Previous,
			enumLL_Up,
			enumLL_Cut,
			enumLL_Esc,
			enumLL_Dn,
			enumLL_Ok,
			enumLL_Title,
			enumLL_NumOfButtons
		}enumButtons_ListaLavori;


	xdata unsigned char i,j;
	xdata float fAux;
	switch(uiGetStatusCurrentWindow()){
		case enumWindowStatus_Null:
		case enumWindowStatus_WaitingReturn:
		default:
			return 0;
		case enumWindowStatus_ReturnFromCall:
			if (pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato>=pJobsSelected_Jobs->ucNumLavoriInLista){
				if (pJobsSelected_Jobs->ucNumLavoriInLista<=defNumLavoriDaVisualizzare)
					pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato=0;
				else
					pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato=pJobsSelected_Jobs->ucNumLavoriInLista-defNumLavoriDaVisualizzare;
			}
			// trasferisco da lista lavori a nvram_struct.commesse...
			vTrasferisciDaListaLavoriAcommesse();
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			return 2;
		case enumWindowStatus_Initialize:
			ucEnableStartLavorazione(1);
			// allineo sempre e comunque il codice prodotto!
			

			// trasferisco i dati da nvram_struct.commesse a lista lavori
			vTrasferisciDaCommesseAlistaLavori();
			i=0;
			while (i<enumLL_NumOfButtons){
				ucCreateTheButton(i); 
				i++;
			}
			vSetStatusCurrentWindow(enumWindowStatus_Active);
			pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato=0;
			pJobsSelected_Jobs->ucNumLavoriInLista=0;
			// validazione lista pJobsSelected_Jobs->..
			i=0;
			while (i<defMaxLavoriInLista){
				// se trovo un record col codice non valido
				// sbianco la lista lavori da qui in poi...
				if (pJobsSelected_Jobs->lista[i].ucValidKey!=defLavoroValidoKey){
					
					while (i<defMaxLavoriInLista){
					    vEmptyLavoroInLista(i);
						i++;
					}
					break;
				}
				pJobsSelected_Jobs->ucNumLavoriInLista++;
				pJobsSelected_Jobs->lista[i].ucPosition=i+1;
				if (pJobsSelected_Jobs->lista[i].uiNumeroPezzi>MAX_QUANTITA)
					pJobsSelected_Jobs->lista[i].uiNumeroPezzi=MAX_QUANTITA;
				if (pJobsSelected_Jobs->lista[i].fOhm>MAX_RES_CAN_MAKE)
					pJobsSelected_Jobs->lista[i].fOhm=MAX_RES_CAN_MAKE;
				else if (pJobsSelected_Jobs->lista[i].fOhm<MIN_RES_CAN_MAKE)
					pJobsSelected_Jobs->lista[i].fOhm=MIN_RES_CAN_MAKE;
				j=0;
				while (j<defMaxCharDescrizioneLavoro){
					if (!isprint(pJobsSelected_Jobs->lista[i].ucDescrizione[j]))
						pJobsSelected_Jobs->lista[i].ucDescrizione[j]=' ';
					j++;
				}
				pJobsSelected_Jobs->lista[i].ucDescrizione[defMaxCharDescrizioneLavoro]=0;
				i++;
			}
			if (pJobsSelected_Jobs->ucNumLavoriInLista>defMaxLavoriInLista)
				pJobsSelected_Jobs->ucNumLavoriInLista=defMaxLavoriInLista;
			// impostazione variabili locali della finestra...
			// defSetWinParam(Win_CP_doSearch,0);
			return 2;
		case enumWindowStatus_Active:
			// GESTIONE DELLA PRESSIONE PULSANTI...
			i=0;
			while (i<enumLL_NumOfButtons){
				if (ucHasBeenPressedButton(i)){
					switch(i){
						// inserimento diretto lotto
						//case enumLL_SingleJob:
						//	vJumpToWindow(enumWinId_LottoDiretto);
						//	return 2;
						case enumLL_Cut:
							/* Indico di eseguire un taglio asincrono. */
							vDoTaglio();
							// non occorre rinfrescare la finestra
							break;
						case enumLL_Mode:
							if (pJobsSelected_Jobs->ucWorkingMode_0L_1LR)
								pJobsSelected_Jobs->ucWorkingMode_0L_1LR=0;
							// vado nel modo lenr solo se abilitato da parametro macchina
							else if (macParms.modo == MODE_LENR)
								pJobsSelected_Jobs->ucWorkingMode_0L_1LR=1;
							break;
						case enumLL_codice:
							return 2;
							
						case enumLL_Title:
						case enumLL_Esc:
							// trasferisco da lista lavori a nvram_struct.commesse...
							vTrasferisciDaListaLavoriAcommesse();
							// chiamo il main menu...
							vJumpToWindow(enumWinId_MainMenu);
							// indico di rinfrescare la finestra
							return 2;

						// richiesto di inserire/modificare un lavoro in lista
						case enumLL_lavoro_1:
						case enumLL_lavoro_2:
						case enumLL_lavoro_3:
						case enumLL_lavoro_4:
							if (ucIsLockedJob(pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i-enumLL_lavoro_1)){
								break;
							}

							// protezione: non posso rappresentare a video pi� di defMaxLavoriInLista lavori
							if (pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i-enumLL_lavoro_1>=defMaxLavoriInLista)
								break;

							// trasferisco da lista lavori a nvram_struct.commesse...
							vTrasferisciDaListaLavoriAcommesse();
							// imposto nel primo parametro della finestra modifica lavori il numero del lavoro che si vuol modificare...
							ucSetWindowParam(enumWinId_ModificaLavoro,0, pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i-enumLL_lavoro_1);
							ucCallWindow(enumWinId_ModificaLavoro);
							return 2;
						case enumLL_desclavoro_1:
						case enumLL_desclavoro_2:
						case enumLL_desclavoro_3:
						case enumLL_desclavoro_4:
							if (ucIsLockedJob(pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i-enumLL_desclavoro_1)){
								break;
							}
							// protezione: non posso rappresentare a video pi� di defMaxLavoriInLista lavori
							if (pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i-enumLL_desclavoro_1>=defMaxLavoriInLista)
								break;
							// imposto nel primo parametro della finestra modifica lavori il numero del lavoro che si vuol modificare...
							// trasferisco da lista lavori a nvram_struct.commesse...
							vTrasferisciDaListaLavoriAcommesse();
							ucSetWindowParam(enumWinId_ModificaLavoro,0, pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i-enumLL_desclavoro_1);
							ucCallWindow(enumWinId_ModificaLavoro);
							return 2;
						case enumLL_Up:
							if (pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato>0){
								if (pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato>defNumLavoriDaVisualizzare)
									pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato-=defNumLavoriDaVisualizzare;
								else
									pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato=0;
							}
							break;
						case enumLL_Dn:
							// devo ricordarmi di visualizzare sempre l'ultimo lavoro, vuoto...
							if (pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+defNumLavoriDaVisualizzare<=pJobsSelected_Jobs->ucNumLavoriInLista){
								if (pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+2*defNumLavoriDaVisualizzare>pJobsSelected_Jobs->ucNumLavoriInLista+1)
									pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato=pJobsSelected_Jobs->ucNumLavoriInLista+1-defNumLavoriDaVisualizzare;
								else
									pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+=defNumLavoriDaVisualizzare;
							}
							break;
						case enumLL_Previous:
							// trasferisco da lista lavori a nvram_struct.commesse...
							vTrasferisciDaListaLavoriAcommesse();
							vJumpToWindow(enumWinId_listaCodici);
							// indico di rinfrescare la finestra
							return 2;

						case enumLL_Ok:
							// trasferisco da lista lavori a nvram_struct.commesse...
							vTrasferisciDaListaLavoriAcommesse();
							if (pJobsSelected_Jobs->ucNumLavoriInLista){
								nvram_struct.ucComingToControlloProduzioneFromJobs=1;
								if (!PrgRunning){
									// salto alla finestra di inserimento ohm/m dei vari fili
									vJumpToWindow(enumWinId_inserisciOhmMfili);
								}
								else{
									if (pJobsSelected_Jobs==pJobsRunning_Jobs){
										// salto alla finestra di setup produzione
										// dato che programma running, non posso modificare ohm/m fili!!!
										vJumpToWindow(enumWinId_setupProduzione);
									}
								}
							}
							// indico di rinfrescare la finestra
							return 2;
						default:
							break;
					}
				}
				i++;
			}
			// PRESENTAZIONE DELLA FINESTRA...


			//strcpy(hw.ucString,"Single job");
			// "rapido" portarlo su menu principale
			//vStringLangCopy(hw.ucString,enumStr20_SingleJob);
			//ucPrintStaticButton(hw.ucString,defLL_Codice_rowUp,defLL_Codice_col+64,enumFontMedium,enumLL_SingleJob,defLCD_Color_Trasparente);

			//vStringLangCopy(hw.ucString,enumStr20_L_Mode+(pJobsSelected_Jobs->ucWorkingMode_0L_1LR?1:0));
			//ucPrintStaticButton(hw.ucString,defLL_Codice_rowUp,defLL_Codice_col+64+6*16U+12U,enumFontMedium,enumLL_Mode,defLCD_Color_Trasparente);
			


			//strcpy(hw.ucString,"   Lista lavori   ");
			vStringLangCopy(hw.ucString,enumStr20_Lista_Lavori);
			ucPrintTitleButton(hw.ucString,defLL_Codice_rowTitle,defLL_Codice_colTitle,enumFontMedium,enumLL_Title,defLCD_Color_Trasparente,1);

			// codice prodotto con intestazione
			// "FILO        LEGA      MANDRINO  NF  TOLL",
			vString40LangCopy(hw.ucString,enumStr40_codefields);
			ucPrintSmallText_ColNames(hw.ucString,defLL_Codice_row-12,defLL_Codice_col);
			// PRIMA DI STAMPARE IL CODICE, NE VISUALIZZO LE VARIE PARTI COME TESTO STATICO PER CAMPIZZARLO

// funzione che stampa a video un testo, usando font medium e colori specificati
			if (!ucTheButtonIsPressed(enumLL_codice)){
				i=0;
				while (i<5){
					strncpy(hw.ucString,his.ucCodice+uc_StartCopy[i],uc_LengthCopy[i]);hw.ucString[uc_LengthCopy[i]]=0;
					ucPrintStaticText_MediumFont_Color(hw.ucString,defLL_Codice_row,defLL_Codice_col+uc_StartCopy[i]*16,defLCD_Color_White,defLCD_Color_Blue);
					i++;
				}
			}			
			
			mystrcpy(hw.ucString,his.ucCodice,sizeof(hw.ucString)-1);
			ucPrintStaticButton(hw.ucString,defLL_Codice_row,defLL_Codice_col,enumFontMedium,enumLL_codice,defLCD_Color_Trasparente);

			//strcpy(hw.ucString," Pos     Num pezzi     Ohm               ");
			vString40LangCopy(hw.ucString,enumStr40_Pos_NumPezzi_Ohm);
			ucPrintSmallText_ColNames(hw.ucString,defLL_Codice_rowNameCols,0);
			i=0;
			while (i<defNumLavoriDaVisualizzare){
				// protezione: non posso rappresentare a video pi� di defMaxLavoriInLista lavori
				if (pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i>=defMaxLavoriInLista)
					break;
				if (pJobsSelected_Jobs->lista[pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i].ucValidKey!=defLavoroValidoKey){
					sprintf(hw.ucString," %-*.*s",19,19,pucStringLang(enumStr20_Aggiungi));
					//sprintf(hw.ucString,"%2i ----     -------",pJobsSelected_Jobs->lista[pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i].ucPosition);
					ucPrintStaticButton(hw.ucString,defLL_Codice_rowCodes+defOffsetRow_listaLavori*i,defLL_Codice_col,enumFontMedium,enumLL_lavoro_1+i,defLCD_Color_Trasparente);
					// se ho trovato il tappo, esco dal loop
					break;
				}
				else{
					fAux=pJobsSelected_Jobs->lista[pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i].fOhm;
					if (fAux>=1000){
						sprintf((char*)pJobsSelected_Jobs->ucAuxString,"%-7.1i",(int)fAux);
					}
					else if ((fAux<1000)&&(fAux>=100)){
						sprintf((char*)pJobsSelected_Jobs->ucAuxString,"%-7.2f",fAux);
					}
					else if ((fAux<100)&&(fAux>=10)){
						sprintf((char*)pJobsSelected_Jobs->ucAuxString,"%-7.2f",fAux);
					}
					else{
						sprintf((char*)pJobsSelected_Jobs->ucAuxString,"%-7.2f",fAux);
					}
					vStringLangCopy(hw.ucString,enumStr20_L_Mode+(pJobsSelected_Jobs->ucWorkingMode_0L_1LR?1:0));
					sprintf((char*)pJobsSelected_Jobs->ucAuxString,"%s %s",pJobsSelected_Jobs->ucAuxString,hw.ucString);
					sprintf(hw.ucString,"%2i %-4i %s",pJobsSelected_Jobs->lista[pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i].ucPosition,pJobsSelected_Jobs->lista[pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i].uiNumeroPezzi,pJobsSelected_Jobs->ucAuxString);
					ucPrintStaticButton(hw.ucString,defLL_Codice_rowCodes+defOffsetRow_listaLavori*i,defLL_Codice_col,enumFontMedium,enumLL_lavoro_1+i,defLCD_Color_Trasparente);

					vStringLangCopy(hw.ucString,enumStr20_Lotto);
					j=strlen(hw.ucString);
					ucPrintSmallText_ColNames(hw.ucString,defLL_Codice_rowCodes+defOffsetRow_listaLavori*i+20,defLL_Codice_col+4);
					mystrcpy(hw.ucString,pJobsSelected_Jobs->lista[pJobsSelected_Jobs->ucNumFirstLavoroVisualizzato+i].ucDescrizione,sizeof(hw.ucString)-1);
					ucPrintStaticButton(hw.ucString,defLL_Codice_rowCodes+defOffsetRow_listaLavori*i+20,defLL_Codice_col+4+((unsigned int)j)*8+4,enumFontSmall,enumLL_desclavoro_1+i,defLCD_Color_Trasparente);

				}
				i++;
			}

			//strcpy(hw.ucString,"Home");
			//ucPrintStaticButton(hw.ucString,defLL_Codice_rowEsc,defLL_Codice_colEsc,enumFontBig,enumLL_Esc,defLCD_Color_Trasparente);
			vAddStandardButtonsOk(enumLL_Previous);

			return 1;
	}
}//lista lavori



// *** 
// *** 
// *** WHEEL ROTATION INVERSION HANDLING BEGINS HERE
// *** 
// *** 
// *** 


typedef struct _type_struct_handle_wheel_inversion{
    unsigned int ui_inversion_active;
    unsigned int ui_test_result;
    unsigned int ui_fpga_hi_version;
    unsigned int ui_fpga_low_version;
    unsigned int fpga_can_handle;
}type_struct_handle_wheel_inversion;
type_struct_handle_wheel_inversion handle_wheel_inversion;

#define def_min_hi_fpga_version_can_handle_wheel_inversion 4
#define def_min_low_fpga_version_can_handle_wheel_inversion 2
// check whether the fpga can handle rotation wheel inversion
// returns 1 if yes, it can handle the inversion
// else returns 0
unsigned int ui_init_fpga_can_handle_wheel_inversion(void){
    // if hi number is bigger than minimum, fpga can surely handle the rotation
    if (handle_wheel_inversion.ui_fpga_hi_version>def_min_hi_fpga_version_can_handle_wheel_inversion){
        return 1;
    }
    // if hi number is lower than minimum, fpga cannot handle the rotation
    if (handle_wheel_inversion.ui_fpga_hi_version<def_min_hi_fpga_version_can_handle_wheel_inversion){
        return 0;
    }
    // if I am here, the hi number is where we changed the feature, let's check the low
    // if low number lower than minimum, fpga cannot handle the rotation
    if (handle_wheel_inversion.ui_fpga_low_version<def_min_low_fpga_version_can_handle_wheel_inversion){
        return 0;
    }
    // if I am here, the hi number is where we changed the feature, 
    // and the low number is equal to or bigger than the one where we changed the feature, so the fpga can handle the inversion
    return 1;
}

// resets the rotation wheel 1 inversion
// returns 0 if the rotation was reset OK
// returns 1 if cannot reset the rotation inversion due to invalid fpga version
// returns 2 if cannot reset the rotation inversion because the fpga does not set the pin
unsigned int ui_zeroOK_reset_rotation_wheel_inversion(void){
    if (!handle_wheel_inversion.fpga_can_handle){
        return 1;
    }
    uc_wheel_one_rotation_invert_0xA5=0;
    if (uc_wheel_one_rotation_invert_0xA5!=0){
        return 2;
    }
    handle_wheel_inversion.ui_inversion_active=0;
    return 0;
}
// sets the rotation wheel 1 inversion
// returns 0 if the rotation was inverted OK
// returns 1 if cannot reset the rotation inversion due to invalid fpga version
// returns 2 if cannot reset the rotation inversion because the fpga does not set the pin
unsigned int ui_zeroOK_set_rotation_wheel_inversion(void){
    if (!handle_wheel_inversion.fpga_can_handle){
        return 1;
    }
    uc_wheel_one_rotation_invert_0xA5=0xA5;
    if (uc_wheel_one_rotation_invert_0xA5!=1){
        return 2;
    }
    handle_wheel_inversion.ui_inversion_active=1;
    return 0;
}

#define def_test_wheel_rotation_inversion
#ifdef def_test_wheel_rotation_inversion
// test rotation wheel inversion
// at the end of the routine
// ui_test_result value     meaning
//     0                    OK
//     1                    invalid FPGA version
//     2                    cannot set inversion, FPGA pin does not set to 1
//     10                   invalid FPGA version
//     20                   cannot reset inversion, FPGA pin does not set to 1
    void v_test_rotation_wheel_inversion(void){
        handle_wheel_inversion.ui_test_result=ui_zeroOK_set_rotation_wheel_inversion();
        if (handle_wheel_inversion.ui_test_result==0){
            handle_wheel_inversion.ui_test_result=ui_zeroOK_reset_rotation_wheel_inversion()*10;
        }
    }
#endif

// rotation wheel inversion
// this works only with fpga versione 4.2 or bigger
void v_init_rotation_wheel_inversion(void){
    memset(&handle_wheel_inversion,0,sizeof(handle_wheel_inversion));
    handle_wheel_inversion.ui_fpga_hi_version=ucFpgaFirmwareVersion_Hi;
    handle_wheel_inversion.ui_fpga_low_version=ucFpgaFirmwareVersion_Low;
    handle_wheel_inversion.fpga_can_handle=ui_init_fpga_can_handle_wheel_inversion();
#ifdef def_test_wheel_rotation_inversion
    v_test_rotation_wheel_inversion();
#endif
    ui_zeroOK_reset_rotation_wheel_inversion();
}
// *** 
// *** 
// *** WHEEL ROTATION INVERSION HANDLING ENDS HERE
// *** 
// *** 
// *** 


