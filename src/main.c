#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <malloc.h>
#include <math.h>
#include <ctype.h>

#include "LPC23xx.h"                        /* LPC23xx/24xx definitions */
#include "type.h" 
#include "target.h"
#include "fpga_nxp.h"
#include "irq.h"

#include "gui.h"
#include "spityp.h"
#include "spiext.h"
#include "spiprot.h"
#include "rtcDS3231.h"
#include "max519.h"
#include "ff.h"
#include "az4186_initHw.h"
#include "az4186_emc.h"
#include "i2c_master.h"
#ifdef def_canopen_enable
    //#warning canopen enabled !!!!
    #include "az4186_can.h"
#endif

// defines per abilitare i vari test
//#define defTestLcd
//#define defTestOutputs

// endless dac test
//#define defTestDac

//#define enable_disk_test
// abilitare questa define per verificare se l'accesso ad fpga (specie ai registri dell'lcd) 
// crea dei glitch che si ripercuotono sulle uscite digitali...
// l'errore era dovuto alla gestione a latch delle uscite digitali nella fpga
//#define def_test_outdig_glitch

//#define defTestAdc

#ifdef defTestAdc
    void v_test_adc(void)
    {
        static unsigned int ui_in_error;
        ui_in_error=0;
        vInitAD7327();
        extern unsigned char ucRefreshAdchannels_Interrupt(void);
        while(ucRefreshAdchannels_Interrupt()){
            ui_in_error=0;
        }
        while(1){
            ui_in_error++;
        }
    }
#endif




// test read rtc ds3231???
//#define def_test_rtc_ds3231    

// generate sawtooth waveforms on both the max519 outputs, on the three max519, period 100ms
//#define defTestMax519



/*
Per convertire da esadecimale a binario:
    hex2bin spiralatrice_arm_lpc.hex
    oppure chiamare il batch ad hoc: az4186_createBin.bat che crea nella directory corrente 
    il file AZ4186FW.BIN a partire dal file spiralatrice_arm_lpc.hex

Here is a summary of the steps for modifying an existing (working) standard (vectors at start of flash, default linker script, etc.) Ride/GCC/ARM application for using it with a bootloader.
(basically, that means leaving a blank space at the beginning of the flash)

1. Build the application 'normally', in Flash mode and using the default linker script. Check that a number (4 or 5) of .ld files are created _in_the_application's_folder_ and identify them using file dates. (read them, they include some useful comments)

2. Change the options for using the generated default script as a "custom linker script":
2.1. Change the application's link option "LD Linker" / "Scripts" / "Use Default Script File" from "Yes" to "No"
2.2. Change the "LD Linker" / "Scripts" / "Script File" option to point to the "secondary/main linker script file" (typically, "STM32F103_64K_20K_FLASH.ld") generated during step 1. (in your application's folder)
2.3. Rebuild the application, check that it still works as before. The result of the link by step 2.2. should be identical to the result of step 1. (each and every byte of it)

3. Change the definition of the FLASH region:
3.1. Modify the memory spaces definitions in the "memory spaces definitions sub-script" (typicaly, "STM32F103_64K_20K_DEF.ld") generated during step 1. (in your application's folder)
For example, to reserve 0x2000 bytes for the bootloader, modify the definition of the Flash region like this:
  FLASH (rx) : ORIGIN = 0x08000000+0x2000, LENGTH = 0x10000-0x2000
DO NOT modify the files in <Ride>\lib\ARM
DO NOT modify the files in <Ride>\scripts
DO NOT add files in <Ride>\scripts
3.2. Build the application
3.3. Load it using your bootloader and observe it executing. At this point you cannot use it without the bootloader.
3.4. If you need to debug it, or just to program and run it from Ride, you will have to include the hex or elf file of the bootloader like if it was a source file of the application, as shown in the example projects for debugging CircleOS applications.

For more information about the management of linker scripts by Ride+RKit_ARM (default, custom, etc.), please refer to the GettingStartedARM document.

For more information about the syntax of the linker scripts and how to modify them, please refer to the LD linker documentation.


*/

//
//
// **********************************************************************
// **********************************************************************
// **********************************************************************
//
// MAJOR NUMBER DELLA VERSIONE FIRMWARE
// MAX 2 CARATTERI
// MAX 2 CARATTERI
//
#ifdef defSimulaEncoder
	#define defMajorNumFirmware_Scheda " X"
#else
	#define defMajorNumFirmware_Scheda " 3"
#endif

//
//
// **********************************************************************
// **********************************************************************
// **********************************************************************
//
// MINOR NUMBER DELLA VERSIONE FIRMWARE
// MAX 2 CARATTERI
// MAX 2 CARATTERI
//
//
#ifdef defSimulaEncoder
	#define defMinorNumFirmware_Scheda "XX"
#else
	#define defMinorNumFirmware_Scheda "36"
#endif

// version history
//
// 3.36
// work almost OK with LPCXpresso
// * MASSIMO NUMERO PEZZI IN PRODUZIONE PORTATO A 99'999  , ERA 9'999
// * minimo diametro mandrino portato a 0.2mm, la risoluzione in inch è 0.001 inch = 0.02mm = 2 centesimi di mm OK
// * minimo diametro filo portato a 0.02mm, la risoluzione in inch è 0.0001 inch = 0.002mm = 2 micron OK
//
// 3.35
// recompiled under LPCXpresso
//
// 3.34
// * CHECKED OK when temperature compensation is not enabled, the temperature compensation parameters are not shown in the product codes archive-show window
// * CHECKED OK: THE PASSWORD IS SAVED ON THE FILE BUT NOT LOADED; corrects BUG that the password was loaded when loading an ini file
// * CHECKED OK the RAM reset can be done only if production is not running
// * CHECKED OK in the manual job window, when measure unit selected is inch, the mm measure unit is displayed instead
// * CHECKED OK when production running, date and time menu is accessible now
// * CHECKED OK when browsing product codes archive, and going down, the move is by 4 codes and not by 3 codes
//
//
// 3.33
// 	- corrects bug in specific resistance alarm display, which was in ohm/mt instead of ohm/feet
// 	- corrects bug in log production window, the measured specific resistence value was always calculated in ohm/mt and not in ohm/ft
// 	- corrects bug that in the direct job mode, the number of pieces already done was not reset to zero and was instead set to the number of pieces of the last work run
//
// 3.32
// 	- corrects display bugs when using imperial sistem units...
//     * in production page: production speed was always in mt/min instead of ft/min, and measured specific resistence was ohm/m instead of ohm/ft
//     * in log wire measure page: specific resistence was ohm/m instead of ohm/ft
//
// 3.31
// 	- la bandella viene azonata, se abilitata, anche durante taglio manuale e durante taglio moncone
//     notare che la specifica iniziale NON prevedeva di azionare la bandella al taglio manuale (cfr. versione 1.31)
//
// 3.30
// 	- il limite massimo della resistenza specifica viene portato a 999.99 ohm/m e 99.999 ohm/feet rispettivamente nelle due unita' di misura
//
// 3.29
// 	- quota di pretaglio viene visualizzata in millesimi di pollice nel sistema imperiale (prima veniva visualizzata in millimetri)
// 	- log visualizza ohm/feet quando si usa sistema imperiale
//
// 3.28
// 	- corregge visualizzazione menu altri parametri produzione che si "incasinava" con spaziatore disabilitato e rettifica abilitata
//
//
// 3.27
// 	- handles spindle grinding
//
// 3.26
// 	- increases max limit of cutting blade from 65 to 85mm as requested via email by Michele Gaiotto
//
// 3.25
// 	- solves bug that when working with coil spacer the spacing parameters weren't copied when changing the tolerance
//
// 3.25
// 	- solves bug that when working with temperature compensation the alfa factor wasn't copied when changing the tolerance
// 	- solves bug that when alfa factor was zero, the compensation was calculated wrong
// 	- solves bug that portataDinamica size was only 4 elements instead of 5 elements
//
// 3.23
// 	- solves bug of incorrect visualization of anybus mac address 
//
// 3.22
// 	se il pretaglio era gi� attivo prima dell'inizio della distanziazione, chiedo di ritornare in pretaglio alla fine della distanziazione
// * when spacing just after cut, the cutter raises to precut, waits 1.0 coils in precut position just to leave the new coil to start correct, then goto spacing position
// * solves bug: when cutting coils with initial spacing, the blade was doing a jerk move when in lower position
//
// 3.21
// * solves bug: when in bush change menu, pressing stop was rising up the blade; this should not happen
//
//
// 3.20
// * some changes on spacing/precut/cut sequences; it seems ok now; tried with coils:
//    spacing from start (start 0%)
//    spacing to end      (end 100%)
//    with normal spacing  (start >0, end <100, not overlapping with precut)
//    with spacing which interferes with precut  (end overlaps with precut)
// * now should visualize temperature in a better way in production control window: now it is displayed on residual time field, toggling between temperature and time
// * when I enter in control production window, I call the alignment between running program and the program selected in joblist so if I press back
//   from production control window I should fall into the correct job
// * when I insert a new program with temperature compensation enabled, the temperature compensation parameters window appears to ask for temperature compensation parameters 
// * when spacing just after cut, the cutter raises to precut, waits 1.3 coils in precut position just to leave the new coil to start correct, then goto spacing position
// * -->CHANGED SOME STRINGS: WORKING MODE INSTEAD OF MODE
// * -->LINGUA IN 3.18 VERIFICARE ED AGGIORNARE MODIFICA LAVORO->INSERISCI DATI LAVORAZIONE
// * risolto bug per cui all'accensione, entrando nel menu delle portate, la quinta portata appariva abilitata (questo capitava anche con la compensazione di temperatura...)
// * mistero, probabile bug latente-->VERIFICARE STRINGA OHM PER TUTTI I FILI
// * solves bug that when working in manual mode and pressing left arrow from control production window, the box was going to joblist instead of going to manual job(flag indicating if we was coming in control window from manual job or job list has been put in non volatile memory)
//
//
//
// 3.19
// * try to solve problem that if precut requested during spacing, the spacing continues until end of coil
// * adjusts cut thresholds...
//
// 3.18
// * thermocouple restored to type J
//
// 3.16
// * thermocouple set to T type just to give a try
//
// 3.15
// * changed default values for k1 and k2 for temperature compensation from 1 to 0, so with a new program the temperature entries are not valid 
// * changed minimum calibration temperature to 30�C: the minimum temperature we can set as calibration point is 30�C
//
// 3.14
// * imported string from Michele Gaiotto's advice
//		"Sens.temp.sconnesso"
//		"Temp. fuori scala-"
//		"Temp. fuori scala+"
//
// 3.13
// * actual temperature appears on production control window, if enabled both on machine parameters and on the product code
//
// 3.12
// * restored-->disables cold junction compensation to check whether the temperatuire reading gets closer to the expected value
// * added message to show when the temperature compensation is warming up
//
// 3.11
// * realize window to set calibration points for temperature compensation; whenvalues are not valid, the background is gray
//    both values of temp and K should be valid to be take in account for alfa calculation; when both couples of values are valid, alfa is the average of the two values
//
// 3.10
// * solves bug right arrow was disappeared in keypad window
// * try to solve bug that product code was randomly changing in product codes window
// * solves bug for which the box was freezing some seconds after startup due to canbus initializations
//
// 3.09
// * added some stuff to handle product codes change of format version
// * added fields t1, Ct1, t2, Ct2 to product code structure
// * check of specific resistance is done after temperature compensation is applied
// * solves bug that we can't set the correction factor of product codes
// * try to solve bug that when saving precut quote with program running, the production speed was overflowing...
// * changed to avoid side effects in spiutils.c --> // uiSetRunningPrg(scp.uiLastRecordFound);
//
// 3.08
// * changes input of temperature coefficient; now user should input k, the resistence factor @100�C
//   alfa = (k@100�C-1)/(100-20)
// * a temperature alarm is showed up:
//     - the first time it appears
//     - always if it appears during production of temperature compensated coils
// * to check if the temperature readings are not good,open the analog input window and check
//   the background color of the temperatures tab: if it is yellow, then the reading are NOT good
//
// 3.07
// * solves bug related to spacer delay time which was set in an incorrect manner
// * when the eeprom is initialized, the parameters are first reset to zero, then set to the values in the ram memory
//
// 3.06
// * some machine parameters can't be set loading parameter file, they are:
//  * spacer type
//  * alarm lamp
//  * fifth range
//  * temperature compensation
//  * cnc serial number
//
// these parameters can be set in the box only using a password furnished by csm, which depends upon the box serial number
// * solves bug: when third language selected, weird text was appearing on the screen
//
// 3.05
// * corrects bug coming from while/for substitution in spiutils.c where entering machine parameters menu hangs all
//
// 3.04
// * now reset ram runs ok, it reset the whole non volatile ram area
// * new language file linguedefault.h, modified by Michele Gaiotto, included 
// * spacer menu access (from production control window) now is possible only if spacer enabled
// * password super user required by:
//     - spacer type
//     - alarm lamp
//     - fifth measure range
//     - temperature compensation
//     - box serial number
// * corregge bug per cui usando i potenziometri allo start con potenziometri le ruote di contrasto �artivano con la velocit� sbagliata
//     correzione in spidbg.c linea 784, la funzione limitcommrpm andava ad alterare in modo errato il numero di giri delle ruote...
// * corregge bug per cui impostando taglio con finecorsa non prendeva lo start
//
// 3.03
// tests coil spacing at start/end positions
//
// 3.02
// versione in cui funziona anybus (trovato bug da correggere su schematico)
//  risolve bug per cui non si salvavano i parametri multipli via ethernet, solo l'ultimo veniva registrato
//
// 3.01
// versione che utilizza anybus
//  bug da risolvere segnalati da Tony:
//   con inserimento codice prodotto manuale, allo start la ruota 2 va troppo veloce, la si deve far scendere di velocit� manualmente
//     notare che la macchina sta lavorando con i potenziometri e non con l'encoder
//   sembra che inserendo una lavorazione manuale nuova, il codice prodotto risultante non parta lento, ma parta con una velocit� alta come se fosse gi� stato programmato
//   verificare che l'associazione col codice prodotto sia corretta quando si lavora con inserimento lavorazione manuale
//
// 3.00
// prima versione portata anche su pc csm
//
// 2.13
// spostata istruzione ui_enable_external_ram(1) dopo che � stata attesa la stabilizzazione fpga ed lcd...
//
//
//
// 2.12
//   verificare se risolto -->correggere bug per cui a variare della temperatura esce allarme filo fuori tolleranza
//   verificare se risolto-->correggere bug per cui impostando parameri programma che � in lista esecuzione aprogramma fermo, all'uscita le modifiche vengono perse perch� sovrascritte da prg running
//   new fields in program: 
//     * the temperature coefficient, should be expressed in 10-6/�C units, eg K=0.0041/�C should be written as 4100
//     * the percentual correction to use when temperature compensation is enabled; this correction is used after the resistance compensation
//       compensation is usef
//
//
// 2.11
//   visualizzazione temperatura anche negativa su menu ingressi analogici
//
//
//
// 2.10
//   prima versione con compensazione di temperatura abilitata e senza simulazioni di sorta; valore di alfa fisso a 0.0041
//   purtroppo la sonda � una pt100 ma il modulo � per le termocoppie... uso il sensore interno del modulo che � pt1000
//   Michele ha collegato una termocoppia di tipo J e sembra funzionare correttamente, adesso verifichiamo con simulatore di termocoppie J se la lettura � accurata
//   risolve bug per cui all'accensione della macchina apparuiva errore timeout sensore temperatura causato da ritardo troppo breve all'avvio prima di inizializzare modulo Phoenix
//   usando il simulatroe di termocoppia, l'errore si assesta nell'ordine di 1.5�C tra 10�C e 40�C
//   il secondo canale letto dal modulo phoenix � stato impostato al valore della temperatura della cold junction
//   aggiunta anche la visualizzazione della temperatura della cold junction alla pagina ingressi analogici
//
// 2.09
//   first try with temperature compensation enabled
//   fixes bug: when super user password was not correct, the box was rebooting!
//
//
// 2.08
//   corregge bug per cui regolando la ruota di contrasto 1 la 2 si fermava e viceversa
//   corregge bug per cui con macchina in allarme la lama veniva continuamente comandata alta
//   corretto: password super user su numero seriale box
//   sembra ok, tenere sotto controllo-->effettuata una modifica (i2c.c), da verificare: errore i2c bus su impostazione quota coltello alto parametri macchina
//   verificato ok-->da verificare: se si verifica allarme interruzione filo durante lavorazione, allo start successivo il dac mandrino non pu� essere modificato, rimane a zero!
//   sembra ok, da verificare: effettuate correzioni per far s� che lo start possa essere dato solo dopo che la lama ha effettuato l'inizializzazione 
//   sembra ok, ma da verificare: effettuate modifiche per impedire lockup i2c bus 
//   verificato ok-->da verificare: come mai non funziona correttamente impostazione realtime clock? come mai quando si legge data/ora in salvataggio parametri macchina talora i2c non funziona correttamente?
//   risolto-->box serial number: --> non viene mantenuto dopo off/on, inoltre non viene salvato in eeprom
//             era dovuto al fatto che se il file dei parametri custom non esiste, non veniva creato, aggiunta opzione per creare file se non esiste
//   risolto-->verificare --> impostazione slowdac in taratura coltello taglio...
//   risolve si spera definitivamente questione i2cbus che saltuariamente si inchioda...
//   risolve bug per cui dopo il primo taglio asincrono con lavorazione in corso, i successivi li faceva senza pausa di pretaglio
//   risolve bug per cui la curva della posizione del coltelo alla fine del taglio aveva un undershoot al di sotto della posizione di riposo, adesso l'undershoot � azzerato
//      -->modifiche in spimis.c per anticipare l'arresto della lama ed evitare l'undershoot
//	 risolve bug per cui dopo lo start, incrementando con encoder la velocit� del mandrino fino a superare quella programmata, la velocit� non si lockava automaticamente al valore nominale 
//   migliorata la gestione della rampa dac del mandrino, e' pi� responsive adesso
//
//
//
// 2.07
// 	bug per con regolazione velocita' tramite encoder la velocita' non si fermava a zero ma riprendeva dal massimo: dovrebbe essere risolto
// 	bug per cui non era possibile impostare quota taglio alta e bassa-->risolto
// 	bug per cui entrando in menu boccola non se ne poteva pi� uscire...-->risolto, tranne che se uno preme stop coltello si alza e procedura rimane bloccata
// 	bug per cui da menu taratura dinamica non sempre le cose funzionavano bene durante la taratura; inoltre premendo misura statica da menu taratura non visualizzava valori corretti
//      --> sembra risolto
// 	da provare-->errore saltuario i2c bus, disabilitate interrupt nested
//
// 2.06
// prima versione provata in macchina
//
// 2.02
// ricompilata in forma strutturata con 60MHz e moduli inithw, i2c, emc
//
// 2.00
// prima versione per piattaforma arm
//
// 1.47
// (in corso)corregge bug per cui superando i 4'000 rpm ruote di contrasto rallentavano il numero di giri
// diminuisce margine per limitazione massimo numero giri ruote contrasto da 30 a 5
// corretta visualizzazione uscite analogiche: il massimo � stato riportato correttamente a 10V, corretta descrizione uscita analogiche
// procedura cambio boccola: se si utilizza taglio con sensore finecorsa, allo start del cambio boccola viene attivata
//  l'uscita di taglio; quando termina la procedura di cambio boccola, l'uscita di taglio viene disattivata
//
//
// 1.46 
// cambiati step max per posizione coltello basso su escursioni da 7 mm
//
//
// 1.45
// CORREGGE GRAVE BUG INTRODOTTO DALLA 1.44 CHE POITEVA PORTARE ALLA CANCELLAZIONE DELLA NAND FLASH
// (era causato dal fatto che la lcdInit chiamava la flash read prima che venisse inizializzato il file sytem!)
// risolve bug per cui quando si usava lingua custom il refresh delle schermate era molto lento
// risolve bug per cui non funzionava memorizzazione numero seriale box a causa di un errore del compilatore C
// risolve bug per cui premendo esc da menu utilit� si risaliva a menu principale...
//
//
// 1.44
// DOVREBBE RISOLVERE BUG PER CUI SALTUARIAMENTE SI CREANO CODICI PRODOTTO "ANOMALI" (es contenenti spazi)
//  l'ipotesi � che i codici anomali si creassero a causa di errori non corretit nella lettura sull nand flash
// -->questa versione abilita gestione ecc codes su nand flash; 
//	- con questi codici � possibile correggere errori sui bit della flash
//	- attualmente posso correggere un errore di 1 bit ogni 256 bytes di codice; dovrebbe essere, 
//    a detta della letteratura in materia, pi� che sufficiente
//	- migliorata gestione cache page nand flash
//	- migliorato controllo di errore nella nand flash
//  - quando si effettua questo aggiornamento, basta che il sistema effettui due salvataggi su nand flash e 
//    la gestione degli ecc codes diviene attiva a tutti gli effetti
//
// 1.43
//
// risolve bug per cui non era + possibile caricare codici prodotto da usb
// dovuto a errore nella chiamata della routine ma anche ad errore per cui caricamentoi poteva entrare in conflitoo
// con gestione modulo anybus...
//

//
// 1.42
//
// mancava traduzione in inglese dell'uscita "BANDELLA"
//
// modificata scritta Lampada in Lamp. Gialla (ed analoga in inglese)
//
// dopo uscita bandella, aggiunta scritta Lamp Rossa (ed analoga in inglese)
//
// in inserimento codice prodotto, numero di fili viene impostato per default ad 1
//    e non condiziona la ricerca codice prodotto,
//    quindi verranno trovati tutti i codici prodotto con un dato diametro filo/mandrino, e che abbiano 1, 2 o 3 fili
//
// introdotto parametro macchina (e stringa corrispondente) "Lampada allarmi":
//   abilitato--> lampada rossa  ON s eallarme presente, OFF else
//				  lampada gialla si accende durante taratura, n pezzi prima della fine, non si accende a fine ciclo
//				  lampada verde si accende via hardware all'accensione mandrino
//   disabilitato--> lampada rossa e verde non sono presenti nel sistema
//				  lampada gialla si accende durante taratura, n pezzi prima della fine, ed a accende a fine ciclo
//
//  durante sequenza reset ram, introdotto messaggio "REBOOT.." che avvisa che la scheda sta esgeuendo reboot
//
//  corretto bug per cui al cambio commessa automatico l'encoder regolazione motori non funzionava pi� se non si faceva lock/unlock
//
//  nella finestra setup produzione, la scritta "salvataggio" ha adesso lo sfondo in magenta per risaltare meglio
//
//  corretto (bug?) per cui nel passaggio automatico da una commessa alla successiva ripartiva da velocit� bassa del mandrino
//    io sono convinto che questa modifica era stata richiesta, comunque � stato ripristinato il comportamento
//    della vecchia spiralatrice che non rallentava nel passaggio automatico
//
//


//
// 1.41
//
// correzione infiniti bugs su ethernet operation
// - se in menu utils e ovunque start non abilitato, non accetto controllo remoto
// - data/ora visualizzati con 2 decimali
// - all'avvio del box, il modulo ethernet viene resettato per evitare di avere indici parametri modbus sballati se box viene riavviato
// - chiede conferma si/no per operazione remota
//
//


// 1.40
//
// proseguono i test per esportazione archivi su modulo anybus
// AGGIUNTA STRINGA "CONTROLLO REMOTO"="REMOTE CONTROL" CHE APPARE QUANDO DA REMOTO SI AGGIORNANO ARCHIVI DEL BOX
// se modulo anybus non risponde e max address=0, allora la comunbicazione viene disabilitata fino alla prossima accensione

//
// 1.39
//
// versione che risolve bug per cui non veniva + accettato carattere '0' (zero)
//  nell'input della stringa che definisce il tipo di filo
//  (questa versione � ancora priva di supporto per modulo anybus)
//
//

//
// 1.38
//
// versione di test esportazione archivi su modulo anybus


// version history
//

//
// 1.37
//
// (proseguono i test ethernet/modbus)
//
//
//

// 1.36
//
// (proseguono i test ethernet/modbus)
// corretto bug per cui stringa password poteva essere impostata a '00000'
// adesso la stringa di password deve essere un numero compreso fra 10'000 e 99'999
// il primo carattere non pu� essere 0
//
//
// test per verificare se il numero di spire di pretaglio � finalmente quello desiderato anche in
// modo L+R, in modo L doveva essere OK anche prima
//		cerca la stringa -->versione 1.36
// test per verificare se il numero di spire di pretaglio � finalmente quello desiderato anche in
// modalit� due fili (sia L che L+R)
//		cerca la stringa -->versione 1.36
//
//
//

// 1.35
//
// CORREZIONE BUG MOLTO IMPORTANTE PER CUI SALTUARIAMENTE LA MACCHINA SI INCHIODAVA AL TERMINE DEL TAGLIO
// ED IL COLTELLO CONTINUAVA A TAGLIARE
//  --> l'errore era provocato dalla chiamata alla commutazione della bandella;
//         questa routine si trova in una altro banco per cui chiamando da interrupt una
//         bank switch succedeva il finimondo!!!!!
//
//  RICORDARSI QUINDI DI NON FARE MAI BANK SWITCH DA INTERRUPT, CONTROLLARE ACCURATAMENTE LE CALL PERCHE'
//   COMPILATORE/LINKER NON DANNO WARNING/ERRORS!!!!
//
//
// (INIZIO test ethernet via uart)
// correzione bug per cui era possibile inserire password vuota; da questa versione la password deve essere
// lunga almeno 5 caratteri
//
// correzione bug per cui con assorbimenti negativi la riga di setup rpoduzione che presenta gli assorbimenti andava
//   a tracimare a sinistra
//
//  correzione bug per cui manipolando la maggiorazione era possibile presentare a video
//   numero di pezzi>9999 in setup produzione, e questo provocava disallineamento finestra a video
//

// 1.34
//
// completata sostituzione delle strcpy con la mystrcpy, in modo da evitare sforamenti nella copia
//   (per vedere se si risolve bug per cui saltuariamente memoria box si corrompe!)
//
// quando si entra in lavorazione manuale provenendo da inserimento lotto, viene portato a 0
//   numero pezzi prodotti e maggiorazione in modo da non "portarsi dietro" numero pezzi da lavorazione precedente
//
// risolto bug introdotto dal 1.33 per cui non era possibile specificare lavorazioni manuali di tipo l+r
//
// risolve bug per cui non funzionava inserimento diametro filo e mandrino in inch
//
// il file az4181p.ini viene salvato con la data memorizzata nel rtc del box
//
// lavorazione manuale lavora su una entry propria che non interferisce con quelle inserite tramite elenco produzione
//  in questo modo quando inserisco lavorazione manuale non vado ad alterare entry esistenti della joblist
//
// nel salvataggio parametri viene inserita anche la tabella fornitori con la parola chiave "forni"
//   la tabella fornitori viene salvata all'interno della sezione dei parametri macchina
//   per ripristinare la tabella fornitori, eseguire "carica parametri macchina"
//
// nel file di salvataggio parametri, la stringa "[End of file]" veniva scritta due volte; adesso la scrive una volta sola
//
// Migliorata la procedura di reset ram del box in modo che sia + pulita
// per azzerare la ram del box:
//   1 accendere tenendo premuto START e MISURA STATICA assieme
//   2 si presenta la schermata di boot
//   3 tener premuto START e MISURA STATICA
//   4 sul touch screen, scegliere EXIT
//   5 sul display appare per un attimo la scritta "REBOOT...WAIT";
//   5 sul display appare la scritta "WAIT RAM RESET";
//   6 attendere;
//   7 quando la ram � resettata, appare la scritta "RAM RESET OK" ed il box fa tre beep;
//   8 riappare la schermata di boot; rilasciare i tasti START e MISURA STATICA
//
// portato da 10 a 5 secondi il tempo di pressione tasti per avviare schermata di boot
//
//
//
//
// 1.33
//
// corretto errore per cui non era possibile passare in modo L in lavorazione manuale
// corretto errore per cui i caratteri '>' e '<' in setup produzione erano sfalsati!
// corretto errore per cui impostando maggiorazione e poi entrando in inserimento manuale veniva riportata anche su
//  nuovo codice inserito
// corretto errore per cui in visualizzazione lunghezza filo necessaria a lavorazione
// non si passava a visualizzazione in feet se veniva cambiata l'unit� di misura da inch a feet
//
// per azzerare la ram del box:
//   1 accendere tenendo premuto START e MISURA STATICA assieme 
//   2 si presenta la schermata di boot
//   3 tener premuto START e MISURA STATICA
//   4 sul touch screen, scegliere EXIT
//   5 attendere; quando la ram � resettata, il box fa tre beep; 
//   6 rilasciare START e MISURA STATICA altrimenti continua a riavviarsi
//
//
//
// inserito blocco per cui se sono in pagina controllo produzione con prg running, e premo freeccia a sinistra,
//   - se provenivo da inserimento lavorazione a menu, vado e modifica resistenza fili
//   - se provenivo da inserimento diretto (manuale) allora rimango fermo in controllo produzione perch� non
//      posso modificare lavorazione manuale se la sto eseguendo....
//
//  fatta modifica che potrebbe risolvere annoso problema per cui regolando con encoder il mandrino in diagnostica
//    vi erano delle strane pause
//
// quando viene letto un programma da nand flash, inserisco sempre il carattere tappo alla fine
// corretto bug per cui facendo modifica posizione di lavorazione e poi premendo cancella la lista si svuotava
// stringa "misura ok" che appare alla fine della taratura portata nelle lingue
// non posso cambiare posizione a lavorazione se non la ho ancora inserita

// 1.32
//
// da controllo produzione, premendo freccia sx si va a lavorazione manuale o inserisci job a seconda di dove si arriva
//
// pulsanti start e stop sono edge sensitive
// se stop premuto, non prende lo start lavorazione
// se emergenza premuta, non lasciava salvare i jobs (inserita gestione per cui lo stop lavora sul fronte)
// risolto bug per cui tarando la posizione di pretaglio rimaneva pulsante pretaglio attivo in giallo...
// su inserimento nuovo programma, imposto per default velocit� W1 a -5%, velocit� W2 a -10%
//	  per modificare default di W1: cerca sui sorgenti la stringa -->posiziono velocit� W1 a -5%
//	  per modificare default di W2: cerca sui sorgenti la stringa -->posiziono velocit� W2 a -10%
// aggiunta nel menu utilit� la procedura di reset ram; al termine, fa ripartire la scheda
//

// 1.31
//
// corretto bug per cui su nuovo codice prodotto non salvava le velocit�
// i tempi bandella sono in centesimi di secondo
// ritardo bandella puo' essere zero senza che sia disabilitata
// la bandella viene pilotata anche se taglio moncone, non viene pilotata se taglio manuale
// se modificata durata attivazione bandella, reinizializzo bandella 
//	cosi' abilitazione/disabilitazione viene fatta al volo
//
// corretto grave errore per cui veniva fatto return invece che jump da finestra codici prodotto!!!!
//
//

// 1.30
//
// INSERITE NUOVE STRINGHE LINGUA:
//   - E' STATA INSERITA IN MEZZO ALLA LISTA L'UNITA' DI MISURA "DECIMI DI SECONDO" SUBITO DOPO "VOLT"
//			"Secondi/10", // decimi di secondo
//   - SONO STATE AGGIUNTE ALLA FINE DELLA LISTA ALTRE STRINGHE DA 20 CARATTERI PER GESTIRE LA BANDELLA
//			"ATTIVAZ.BANDELLA",
//			"RITARDO BANDELLA",
//
//  corretto errore per cui se con prg running andavo a selezionare un codice prodotto diverso,
//    prendeva una velocit� massima sbagliata (cambiava il punattore al prg running!!!)
//
//  corretto piccolo bug t9 per cui se si passava da t9 a normale, perdeva l'ultima carattere
//  inserito
//
//
//  corregge errore per cui pezzi prodotti non venivano aggiornati se si dava stop
//  corregge errore per cui si poteva entrare in lista jobs anche se 0 codici selezionati
//	corretto errore per cui pulsante visualizza codici e cancella codici appariva e spariva a sproposito
//  corretto errore per cui se lavorazione in corso, si faceva stop, si spegneva e riaccendeva la macchina,
//		non tornava in menu controllo produzione
//  corretto errore per cui dopo aver effettuato taratura misura statica, la finestra di visualizzazione misura statica
//     spariva finch� non si usciva e poi si rientrava nella finestra di controllo produzione
//  inserimento tolleranza filo: adesso appare la finestra che visualizza la tabella delle tolleranze;
//    premendo su uno dei pulsanti delle tolleranze viene memorizzata la tolleranza corrispondente
//    (prima era necessario inserire un numero tra 0 e 9)
//  salvataggio setup di produzione: aggiunta pausa di 1 secondo dopo stabilizzazione dac
//    per fare in modo che setup venga salvato quando la velocit� in m/min si � stabilizzata
//    ed evitare che non sia memorizzata col valore corretto
//    aggiunto anche un messaggio in giallo che evidenzia lo stato di salvataggio
//    finch� non ha finito il salvataggio (dura comunque poco) i pulsanti a display sono disabilitati
//    (in questo modo non si pu� uscire dal menu n� premere pulsanti finche' il salvataggio non � completo)
//  il tasto "visualizza tutti i codici" viene visualizzato solo quando serve
//  migliorato passaggio dei dati da lista lavori a lista lavori in esecuzione (se programma running,
//    si parte da commessa running e non da commessa numero 0)
//  se da finestra lavori si preme ok e programma � running:
//    - se il lavoro in modifica � quello running, si va in pagina di controllo produzione (non in pagina ohm/m fili!)
//    - altrimenti se si sta modificando un lavoro che non � quello running,,si rimane in pagina lista lavori
//  se programma non � running, premendo ok si va in pagina inserimento ohm/m fili
//  inserita gestione bandella:
//    - la DECIMA uscita digitale � l'uscita bandella (quella successiva all'uscita lampada)
//        (la nona � stata saltata perch� riservata a spaziatore)
//    - inseriti due parametri macchina:
//        ritardo attivazione bandella [s/10] indica quanto tempo dopo la fine del job viene comandata ad 1 l'uscita bandella
//        durata attivazione bandella [s/10] indica per quanto tempo dopo il ritardo bandella viene mantenuta ad 1 l'uscita bandella
//
//
//

// 1.29
//   corregge errore per cui se resistenza superiore a 2'000 ohm la misura
//   statica la visualizzava senza decimali; adesso fino a 10'000 ohm viene visualizzato un decimale
//
//   modificata selezione codici prodotto: il numero di fili fa da filtro
//    Nella 1.28 il numero di fili entrava nel filtro solo se erano inseriti anche il diametro filo o il diametro mandrino
//
//   corretto errore per cui crc dei parametri macchina non era calcolato correttamente
//    per cui poteva capitare che non venisse rilevato un errore crc
//    potenziato il crc: adesso ne uso uno polinomiale che garantisce miglior rilevamento degli errori
//    in precedenza, se flash era tutta a 0xff, crc poteva anche non accorgersi del problema
//
//   qualche modifica per cercare di risolvere problema del numero di spire di pretaglio (da provare)
//
//   verificato che dalla versione 1.28 gli assorbimenti vengono visualizzati anche se negativi
//

// 1.28
//     gestisce lista di max 3 codici prodotto, ciascuno con la sua job list
//      per passare da un codice prodotto ad un altro senza cancellare la job list
//
//     finestra "produzione-altri parametri"
//			allargata a 20 caratteri la dimensione del campo descrizione parametro
//
//

// 1.27 prima versione rilasciata a beta tester



//
//
// **********************************************************************
// **********************************************************************
// **********************************************************************
//
// STRINGA CHE IDENTIFICA LA SCHEDA
//
//
#define defStrVersioneFirmware_Scheda "AZ4186 ver."
//
//
// **********************************************************************
// **********************************************************************
// **********************************************************************
//
// STRINGA CHE DEFINISCE LA VERSIONE
// inizia con AZ4186 e finisce col carattere @
// "AZ4186 ver. XX.xx Mon gg aaaa hh:mm:ss @"
// ad es.:
// "AZ4186 ver.  2.00 Dec 14 2011 14:56:07 @"
const unsigned char __attribute__((section("user_fw_version"))) ucFirmwareVersion[]=defStrVersioneFirmware_Scheda" "defMajorNumFirmware_Scheda"."defMinorNumFirmware_Scheda" "__DATE__" "__TIME__" @";

//
//
// **********************************************************************
// **********************************************************************
// **********************************************************************
//
//


//
//
//
// versione 0.27 del 17 luglio 2009
// - corretto errore per cui lingua custom non veniva caricata correttamente
//    l'errore era in myFlashFs.c --> veniva usata page_size invece di page_data_size!!!!!
// - la scritta "vedi" codici prodotto � stata allargata a " vedi "
// - in finestra allarme, sostituita stringa su pulsante di reset allarme: da "Esc" a "RESET"
// - per fare taratura pezzi, basta premere pulsante misura statica, non occorre tenerlo premuto fino alla fine della taratura
// - inserita visualizzazione metri filo necessari per job/job list
// - inserite anche le stringhe di dagnostica nella lingua
// - corretto errore per cui in setup produzione non era visualizzata la velocit� "vera"
//     ma quella memorizzata, che non � detto coincida con quella ipotetica calcolata...
// - periodo di refresh della velocit� mandrino portato a 200ms, diminuito filtro per avere aggiornamento + rapido...
// - corretto errore in calcolo stima dei tempi, non veniva azzerato tempo corrente esecuzione
//    resistenza se motore fermo...
// - sistemati i menu "lista di produzione"
// - alla fine di una lavorazione fa il purge della joblist
// - modificati i menus di gestione lista codici
// - corretto errore per cui cancellava entry in lista produzione anche se si rispondeva no
//
//
//
//
//
//
// versione 0.26 del 29 giugno 2009
// - menu cambio boccola: posso entrarci solo se prg NON running!!!
//
//
//
//
//
// versione 0.23 del 12 giugno 2009
// - gestisce boot da flash secondaria; si accede premendo start/stop all'avvio
// - gestisce update firmware: inserire chiave usb che contenga i files az4181.bic, .bi0, ... .bi5 in root della chiavetta
//   e seguire la procedura a video
// - gestisce salvataggio dei pars
// - modificata routine di taglio, adesso dovrebbe essere evitato l'undershoot
//
//
//
// versione 0.22 del 15 maggio 2009
// - versione con fpga che fa automaticamente acquisizione dai canali a/d, e ne fa la media su 128 acquisizioni (2us/ch*8ch/loop*128loop/filtro)-->2ms/filtro
//   quindi otteniamo molto rapidamente la media su 2ms
//
// versione 0.21 del 7 maggio 2009
// - se lista comandi ad lcd troppo lunga, viene spezzata in due
// - completato menu taratura sensore di taglio
// - migliorata grafica menu ingressi/uscite digitali
// - inserita gestione sensore di taglio anche in interrupt
// - aumentata dimensione heap di 1000bytes per evitare problemi con schermate dense di pulsanti
//
// versione 0.20 del 30 aprile 2009
// - menu cambio boccola; se si spegne con macchina in quel menu, riparte da l�
//
// versione 0.19 del 28 aprile 2009
// - corretto grave errore per cui nella misura in interrupt veniva usata la taratura del canale statico invece di
//    quello dinamico (canale 0 invece di 1)
//    -->spimis.c linea 769 -->pTaratura=&(macParms.tarParms[N_CANALE_OHMETRO_DYN][actDynPortata]);
// - taglia il primo moncone dopo circa 60mm, non taglia spirali nell'attesa di tagliare il primo moncone
// - � possibile cambiare la tolleranza del filo direttamente dalla pagina produzione
//   --> se il codice prodotto con la tolleranza modificata non esiste, viene creato
//       con i parametri (velocit� etc) del programma corrente
// - da pagina diagnostica � possibile impostare con encoder gli rpm frizione/mandrino
// - migliorata grafica pagina diagnostica
// - font da usare per scritte bitmap: oloron 12
//
// versione 0.18 del 24 aprile 2009
// - corretto bug per cui i bitmap button non si schiacciavano bene: era sbagliata la definizione dell'area del button
//
// versione 0.16 del 22 aprile 2009
// - NUOVA VESTE GRAFICA
// - correzione errore per cui allarme filo aggrovigliato inchiodava macchina
// - correzione errore per cui non funzionava taratura resistenze
// - correzione errore per cui non funzionava aggiustamento assorbimento frizione 2
// - migliorato log produzione
//
// versione 0.15 del 16 aprile 2009
// - inserimento quinta portata dinamica...
//
// versione 0.14 dell' 1 aprile 2009
// - inizio inserimento lotto rapido: si inserisce direttamente diametri, numero pezzi ohm e via
// - anche struttura his, che contiene codice prodotto corrente sotto forma di stringhe,
//   � stata allocata in modo statico in xdata
// - per ogni lista lavori � possibile specificare se deve essere eseguita in modo L o in modo L+R
//       lavoro in modo L+R se modo L+R abilitato da setup e se la lista lavori corrente ha modo L+R abilitato
// - inserito parametro macchina che abilita passaggio automatioc da una commessa alla successiva
//       tutti i lavori che appartengono alla stessa lista ereditano la propriet� di passaggio automatico
//		 direttamente dal parametro macchina
// - inizio inserimento diagnostica spezzoni
//
//
//
// versione 0.13 del 27 marzo 2009
//  inseriti in indirizzi fissi di memoria xdata:
// - nvram_struct.commesse
// - lingua
// - unit� di misura
// - resistenze specifiche fili
// - tabella fornitori
// inizio gestione potenziometro di regolazione delle velocit�
//
// versione 0.12 del 23 marzo 2009
//  inizio programmazione IAP:
//		definita stringa della versione in posizione fissa
//		provata programmazione flash iap e funziona
//		rimane da inserire interfaccia con file vdip per prelevare codice di programmazione
// versione 0.11 del 19 marzo 2009
// - inserita gestione password e gestione versioni parametri macchina< verificare perch�
//    si sono resettati pars calibra lcd e pars tarature: forse ho attivato emulatore troppo presto???
// versione 0.10 del 18 marzo 2009
// - corretta visualizzazione pagina produzione: se potenziometri in lock, visualizzo
//     con font grande la velocit� di produzione [m/min] ed il numero di pezzi prodotti
// - verificato funzionamento con misura ohmica, in particolare verificati allarmi tempo insufficiente e salto misura ohmica
// - verificato che tempo in interrupt non � mai superiore a 2.5ms, cio� al 50% di overhead
// - sistemati bug minori in visualizzazione tabella fornitori
// versione 0.9 del 27 febbraio 2009
// - aggiunte finestre di taratura strumenti
// - risolto annoso bug per cui con molti refresh lcd veniva persa pressione touch (si sentiva solo il rilascio)
//   (forzata rilettura touch dopo un refresh lcd, con controllo su posizione di touch)
// - inizio inserimento utilities
//    da fare--> non comandare uscite se sono in pagina diagnostica...
// versione 0.8 del 29 gennaio 2009
// - quasi completata finestra lista lavori/modifica lavori; inizio procedura di start lavorazione
// versione 0.7 del 16 gennaio 2008
// - aggiunta gestione button da 20 caratteri (scaricare immagine su flash del display!)
// versione 0.6 del 12 gennaio 2008
// - inserimento primitive per nuova interfaccia az4181
// 	- primitive di ricerca codici programma con caratteri jolly, direzione impostabile
// versione 0.5 del 19 dicembre 2008
// - inserita gestione pulsanti con effetto touch
// - inizio inserimento gestione finestre, input da tastierino numerico
// - fatto: taratura 'T' deve essere attiva solo se programma running
// - fatto: taratura 'T' non ha effetto se misura resistenza infinita
// versione 0.4 dell'11 dicembre 2008
// - provata taratura a due punti degli strumenti di misura: ok
// - corretto errore per cui coefficiente di correzione tramite taratura statica non veniva memorizzato
// - corretto errore in inserimento programmi
// - verificata qualit� delle misure
// - corretto errore per cui numero pezzi avviso andava sempre a 0 (era compilatore che sbagliava a valutare
//   unsigned char <0)
// versione 0.3 del 5 dicembre 2008
// - inserita taratura a due punti degli strumenti di misura
// - visualizzazione a display della misura statica mostra solo cifre significative a seconda della portata scelta
//    (0@1x,1@10x,2@100x,3@1000x)
// - inizializzazione dei parametri macchina imposta tutti i campi di taratura (per eliminare NAN)
// - se motore fermo la velocit� in metri/minuto visualizzata � 0
// - se premo misura statica e sono nel menu taratura dinamica, mi visualizza
//   il valore in ohm di quello che si trova fra i contatti della ruota dinamica, utile per le tarature
//   dell'ohmetro dinamico
// - backspace=f5 per tarare la macchina
// - portato a 10 SECONDI TIMEOUT PER FARE RESET nvram_struct.commesse PREMEENDO START/STOP ALL'AVVIO
// - corretto errore per cui il calcolo di ro nella misura ohmica in interrupt non funzionava
// - corrette scritte in cui compariva carattere fi
// versione 0.2 del 3 DICEMBRE 2008
// - corretto errore per cui se actPrg==runningPrg succedevano problemi per cui le modifiche ad actPrg
//   non si riflettevano su runningPrg, ad es la v delle ruote di contrasto non si adeguava
// - corretto errore su isteresi delle portate della misura statica (non va bene 10% come minimo, deve essere minore del 10%)
// - se pulsante di misura statica non premuto, visualizza --- invece che 0
// - corretto errore per cui non funzionava impostazione dac i2c bus (mandrino, ruote contrasto)
// - premendo all'avvio sia start che stop per 3 secondi, si cancellano le nvram_struct.commesse
// - premendo all'avvio sia start che prova ohmica per 3 secondi, si cancella la flash (programmi parametri macchina calibrazione lcd)
//       e le nvram_struct.commesse
// - corretta portata della misura dinamica, adesso 8V=5/50/500/5000 ohm (prima era 5V=...)
// - abbassato tempo di startup fpga lcd da 1000ms a 200ms
// - fa beep solo se viene premuto tasto valido, non ogni volta che si preme touch...
//
//
// versione 0.01 del 27 novembre 2008
// - salvataggio in flash dei parametri macchina (comprese le tarature) (da provare)
// - salvataggio in flash della calibrazione lcd						(da provare)
// - simulazione modo lenr per prove a banco (da provare)
// - uscita dac ruota contrasto 2 da secondo canale secondo max519 (da provare)
// - impostate max 100 nvram_struct.commesse, che vengono mantenute in memoria ram...
// - guadagno di taratura viene moltiplicato per 1024, per fare divisione pi� rapidamente in interrupt
// - guadagno di actCorrOhmica viene moltiplicato per 1024, per fare divisione pi� rapidamente in interrupt
// - sistemati caratteri freccia (messo > <) e diametro (messo D, fi non c'� nel font medium)
// - sistemata memorizzazione velocit� di produzione




typedef struct _TipoStructFirmwareVersion{
	unsigned char *pStringVersion;
	unsigned int uiMajorNum,uiMinorNum;
}TipoStructFirmwareVersion;
TipoStructFirmwareVersion xdata firmwareVersion;


#if 0
// definizione della dimensione del pool di memoria heap a disposizione
#define defMemPoolSize 3500
// allocazione della memoria dinamica del sistema
unsigned char xdata mempool_array[defMemPoolSize];
#endif
unsigned long xdata ulWaitCycles;

xdata TipoExamineBootReset examineBootReset[2];
xdata unsigned char ucBooting;


typedef struct _TipoHandleTimeInterrupt{
	unsigned char ucSaveCntInterrupt,ucActCntInterrupt;
	unsigned int uiContaMsFpgaBoot;
	unsigned int uiFpgaBootTimeElapsed;
}TipoHandleTimeInterrupt;
xdata TipoHandleTimeInterrupt hti;

// per quanti ms devo premere start/stop oppure start/prova ohmica
// all'avvio per fare reset???
#define defTimeResetBoot_PressButtonMs 5000
// tempo di boot della fpga scheda az4181
#define defTimeBoot_FpgaAZ4181Ms 500





volatile unsigned int time,nextTime;

#ifdef defTestOutputs
	void vTestOutputs(void){
		unsigned char myMsg[120];
		unsigned char ucByteWritten_Low,ucByteWritten_Hi;
		unsigned long ulNumLoops,ulNumErr;

		WDMOD=0X00;
	// watch dog feed
		WDFEED=0XAA;
		WDFEED=0X55;
		WDTC=0xffff;
		ulWaitCycles=0;
		while(ulWaitCycles<10000){
		// watch dog feed
			WDFEED=0XAA;
			WDFEED=0X55;
			ulWaitCycles++;
		}
		vLCD_Init();
		vInitListDisplayElements();
		vResetWindows();
		initTimer();
		ulNumLoops=0;
		ulNumErr=0;
		while(1){
			hti.ucSaveCntInterrupt=ucCntInterrupt;
			hti.uiFpgaBootTimeElapsed=1000;
			hti.uiContaMsFpgaBoot=0;
			while(1){
				if (hti.ucSaveCntInterrupt!=ucCntInterrupt){
					hti.ucSaveCntInterrupt=ucCntInterrupt;
					hti.uiFpgaBootTimeElapsed++;       
					hti.uiContaMsFpgaBoot++;
					ucByteWritten_Low=hti.uiContaMsFpgaBoot&0xFF;
				// non scrivo sul bit 7 della parte alta perch� lo uso per misurare la durata della
				// funzione di interrupt timer0
					ucByteWritten_Hi=((~ucByteWritten_Low)&0x7F);
					OutDig=ucByteWritten_Low;
					Outputs_Hi=ucByteWritten_Hi;
					if ((OutDig==ucByteWritten_Low)&&(Outputs_Hi==ucByteWritten_Hi)){
					}
					else{
						ulNumErr++;
					}
					ulNumLoops++;
					if (hti.uiFpgaBootTimeElapsed*5>1000){
						hti.uiFpgaBootTimeElapsed=0;
						ucClearScreen();
						ucCreateTheButton(1);                
						if (ulNumErr==0){
							sprintf(myMsg,"L%02X(%02X),H%02X(%02X)",((int)OutDig)&0xFF,ucByteWritten_Low,((int)Outputs_Hi)&0xff,ucByteWritten_Hi);
							ucPrintStaticButton(myMsg,100,10,enumFontMedium,1,defLCD_Color_Trasparente);
						}
						else{
							sprintf(myMsg,"KO L%02X(%02X),H%02X(%02X)",((int)OutDig)&0xFF,ucByteWritten_Low,((int)Outputs_Hi)&0xff,ucByteWritten_Hi);
							ucPrintStaticButton(myMsg,100,10,enumFontMedium,1,defLCD_Color_Yellow);
						}
						sprintf(myMsg,"err:%X, n:%X",ulNumErr,ulNumLoops);
						ucPrintStaticButton(myMsg,140,10,enumFontMedium,1,defLCD_Color_Trasparente);
						vRefreshLcd(enumRefreshLcd_Timeout);
					}
				}
			}
		}
	}// vTestOutputs
#endif


#ifdef defTestLcd 
	// programma di test lcd
	void vTestLcd(void){
		unsigned char myMsg[120]; 
        unsigned int ui_elapsed_minuti;
        unsigned int ui_elapsed_secondi;
        unsigned int ui_elapsed_decimi;
        unsigned int uc_new_interrupt;
		WDMOD=0X00;
	// watch dog feed
		WDFEED=0XAA;
		WDFEED=0X55;
		WDTC=0xffff;
		ulWaitCycles=0;
		while(ulWaitCycles<100){
		// watch dog feed
			WDFEED=0XAA;
			WDFEED=0X55;
			ulWaitCycles++;
		}
		initTimer(); 
		vLCD_Init();
		vInitListDisplayElements();
		// unnecessary, it hangs on non_existent nand vResetWindows();
		vVisualizeSplashScreen();
		hti.ucSaveCntInterrupt=ucCntInterrupt;
		hti.uiFpgaBootTimeElapsed=1000U;
		hti.uiContaMsFpgaBoot=0;
        ui_elapsed_minuti=0;
        ui_elapsed_secondi=0;
        ui_elapsed_secondi=0;
        ui_elapsed_decimi=0;
        uc_new_interrupt=ucCntInterrupt;
        hti.ucSaveCntInterrupt=uc_new_interrupt;
    
		while(1){
            uc_new_interrupt=ucCntInterrupt;
            while(hti.ucSaveCntInterrupt!=uc_new_interrupt){
                hti.uiFpgaBootTimeElapsed++;       
                hti.ucSaveCntInterrupt++;
            }
            if (hti.uiFpgaBootTimeElapsed>=100/PeriodoIntMs){
                void vPrintTesto(unsigned int uiRiga,unsigned int uiColonna, unsigned char xdata *pTesto);
                void vRefreshScreen(void);
                hti.uiFpgaBootTimeElapsed-=100/PeriodoIntMs;
                if (++ui_elapsed_decimi>=10){
                    ui_elapsed_decimi-=10;
                    if (++ui_elapsed_secondi>=60){
                        ui_elapsed_secondi-=60;
                        ui_elapsed_minuti++;
                    }
                }
                hti.uiContaMsFpgaBoot++;
                ucClearScreen();
                sprintf(myMsg,"AZ4186 loop %i:%02i:%i",ui_elapsed_minuti,ui_elapsed_secondi,ui_elapsed_decimi);
                vPrintTesto(20,20, myMsg);
                vRefreshScreen();
            }
			// watch dog feed
			WDFEED=0XAA;
			WDFEED=0X55;
	        
		}
		while(1){
		}
	}//void vTestLcd(void)
#endif

// routine che inizializza i valori dac verso mandrino e ruote di contrasto, e soprsttutto verso il coltello, per evitare che oscilli
// continuamente finch� non viene messo a zero...
void vInitializeTheDacValues(void){
	// Azzero le uscite dei dac motore e ruote di contrasto. 
	// occhio che il dac del coltello va impostato a 128 perch� � bipolare
	SendByteMax519(addrDacMandrino,0,128);
	SendByteMax519(addrDacFrizione1,0,0);
}


// attesa che fpga faccia boot: i comandi eventualmente spediti ad fpga prima che finisca il boot
// vengono persi, per cui � importante essere certi di inizializzare il display
// DOPO che fpga ha terminato il boot
void vWaitBootFpga(void){
	// azzero struttura che permette di controllare se richiesto reset al boot
	memset(&examineBootReset,0,sizeof(examineBootReset));
	// pausa in attesa che fpga sia operativa...
	// attenzione!!!! se si toglie questo ritardo il sistema NON parte correttamente!!!
	// aspetto che la mia fpga si stabilizzi...
	hti.ucSaveCntInterrupt=ucCntInterrupt;
	hti.uiFpgaBootTimeElapsed=0;
	hti.uiContaMsFpgaBoot=0;
	// aspetto di verificare se richiesto reset ram/flash
	// e che la fpga sia stabile...
	while(  (!examineBootReset[0].ucBootResetExamined)
		   ||(!hti.uiFpgaBootTimeElapsed)
		 ){
		 hti.ucActCntInterrupt=ucCntInterrupt;
		 if (hti.ucSaveCntInterrupt!=hti.ucActCntInterrupt){
			if (hti.ucSaveCntInterrupt<hti.ucActCntInterrupt){
				hti.uiContaMsFpgaBoot+=hti.ucActCntInterrupt-hti.ucSaveCntInterrupt;
			}
			else{
				hti.uiContaMsFpgaBoot+=(256U-hti.ucActCntInterrupt);
				hti.uiContaMsFpgaBoot-=hti.ucSaveCntInterrupt;
			}
			hti.ucSaveCntInterrupt=hti.ucActCntInterrupt;
			if (hti.uiContaMsFpgaBoot>defTimeBoot_FpgaAZ4181Ms/PeriodoIntMs)
				hti.uiFpgaBootTimeElapsed=1;
		 }
		if (!examineBootReset[0].ucBootResetExamined){
			// se start o stop non premuti, fine controllo
			if (  !(InpDig & IDG_START)
			    ||!(InpDig & IDG_MSTA)
			  )
			  examineBootReset[0].ucBootResetExamined=1;
			// se non � gi� stato fatto il controllo al boot...
			else if ( hti.ucActCntInterrupt!=examineBootReset[0].ucSaveCntInterrupt ){
				// se start e stop premuti per pi� di 2 secondi all'avvio, reset...
   				if (examineBootReset[0].ucSaveCntInterrupt<hti.ucActCntInterrupt){
					examineBootReset[0].uiCntLoop+=hti.ucActCntInterrupt-examineBootReset[0].ucSaveCntInterrupt;
				}
				else{
					examineBootReset[0].uiCntLoop+=(256U-hti.ucActCntInterrupt);
					examineBootReset[0].uiCntLoop-=examineBootReset[0].ucSaveCntInterrupt;
				}
				examineBootReset[0].ucSaveCntInterrupt=hti.ucActCntInterrupt;
   				if (examineBootReset[0].uiCntLoop>=(defTimeResetBoot_PressButtonMs/PeriodoIntMs)){
   					examineBootReset[0].ucBootResetExamined=1;
					examineBootReset[0].ucBootResetNecessary=1;
				}
			}
		}

	}
}//void vWaitBootFpga(void)

    // funzione che dato un testo piccolo a display lo traduce in mappa icone o bitmap
    // in rosso su sfondo bianco
    void vPrintSmallTesto_Red(unsigned int uiRiga,unsigned int uiColonna, unsigned char xdata *pTesto){
        pParamsIcon->uiYpixel=uiRiga;
        pParamsIcon->uiXpixel=uiColonna;
        pParamsIcon->ucFont=0; // font 16x16
        pParamsIcon->pucVettore=pTesto;
        pParamsIcon->ucIconColor = defLCD_Color_Red; 
        pParamsIcon->ucBackgroundColor = (defLCD_Color_White);
        vWriteIconElement(pParamsIcon);
    }


    // refresh dello screen di startup, che visualizza il logo CSM
    void vRefreshScreen_Startup(void){
        vCloseListDisplayElements();
        invio_lista();
        LCD_LoadImageOnBackgroundBank(18);
        applicazione_lista();
        LCD_toggle_bank();
        vInitListDisplayElements();
    }
// test read rtc ds3231???
#ifdef def_test_rtc_ds3231    
    void v_test_read_rtc_ds3231(void){
        unsigned int ui_prev_secondi;
        extern TipoStructRTC rtc;
        char mymsg[64];
        ui_prev_secondi=0xffff;
    // inizializzazione i2c bus
        vInitI2c();
    // init rtc for fat gettime
        vInit_RtcDS3231();
    
    // inizializzazione lcd
        vLCD_Init();
        // inizializzazione lista elementi display
        vInitListDisplayElements(); 

        while(1){
            vReadTimeBytesRTC_DS3231();
            if (rtc.ucSecondi!=ui_prev_secondi){
                ui_prev_secondi=rtc.ucSecondi;
                sprintf(mymsg,"%i:%i:%i",(int)rtc.ucOre,(int)rtc.ucMinuti,(int)rtc.ucSecondi);
                vPrintSmallTesto_Red(10,90, mymsg);
                vRefreshScreen_Startup();        
            }
        }
    }

#endif
// funzione di stampa a video messaggi diagnostici
// ingresso: indice riga su cui stampare
//              attualmente deve essere compreso fra 0 e 20
//            puntatore alla stringa da stampare
void v_print_msg(unsigned int row,char *s){
    if (row>20)
        row=20;
    vPrintSmallTesto_Red(row*10,10, s);
}

// generate sawtooth waveforms on both the max519 outputs, on the three max519, period 100ms
#ifdef defTestMax519
    // endless max519 test: outputs a sawtooth waveform with circa 100ms period
    void vTestMax519(void){
        #define def_sawtooth_period_ms 100
        #define def_num_bit_max519 8
        #define def_fs_max519 ((1<<def_num_bit_max519)-1)
        unsigned int ui_values[6];
        unsigned int i;
        extern unsigned char ucLastCommandMandrino;
        // inizializzazione lcd
        vLCD_Init();
        // inizializzazione lista elementi display
        vInitListDisplayElements();      
        // delays each sawtooth by 1/n
        for (i=0;i<sizeof(ui_values)/sizeof(ui_values[0]);i++){
            ui_values[i]=i*def_fs_max519/(sizeof(ui_values)/sizeof(ui_values[0]));
        }

        vPrintSmallTesto_Red(10,90, "testing max519");
        vRefreshScreen_Startup();      
        
    // loop generating sawtooth
        while(1){
            // update dac outputs
#warning "\n*************\ndebug output hi @ 0x40 per tempistica i2c\n*************\n"
	Outputs_Hi|=0x40;        
            ucLastCommandMandrino=ui_values[0];
            ucSendCommandPosTaglioMax519_Main(ui_values[1]);
	Outputs_Hi&=~0x40;        
            // delay so that the entire sawtooth lasts for def_sawtooth_period_ms
            v_wait_microseconds((def_sawtooth_period_ms*1000)/def_fs_max519/4);
	Outputs_Hi|=0x40;        
            SendByteMax519(addrDacFrizione1,ui_values[2],ui_values[3]);
	Outputs_Hi&=~0x40;        
            // delay so that the entire sawtooth lasts for def_sawtooth_period_ms
            v_wait_microseconds((def_sawtooth_period_ms*1000)/def_fs_max519/4);
	// Outputs_Hi|=0x40;        
            // SendByteMax519(addrDacColtello,ui_values[4],ui_values[5]);
	// Outputs_Hi&=~0x40;        
            // delay so that the entire sawtooth lasts for def_sawtooth_period_ms
            v_wait_microseconds((def_sawtooth_period_ms*1000)/def_fs_max519/4);
            // update dac values variables
            for (i=0;i<sizeof(ui_values)/sizeof(ui_values[0]);i++){
                if (++ui_values[i]>def_fs_max519)
                    ui_values[i]=0;
            }
        }
    }
#endif

#ifdef enable_disk_test
    void v_disk_test(void){
        static FATFS fatfs;
        disableIRQ();
        // never enable interrupts here, else the system hangs! you should before set the new int vectors
        //enableIRQ();
        vAZ4186_InitHwStuff();
        // inizializzazione lcd
        vLCD_Init();
        LCD_Beep();
        // inizializzazione lista elementi display
        vInitListDisplayElements();
        vPrintTesto(20,10, "hello");
        vRefreshScreen();
     // init i2c for ds3231 communications
        vInitI2c();
    
    // init rtc for fat gettime
        vInit_RtcDS3231();
    
// mount file system
        f_mount(0, &fatfs);		// Register volume work area (never fails) 

// load mac parameters
        vLoadMacParms();
        while(1){
        }
    
    }
#endif

#ifdef def_test_outdig_glitch
void v_test_refresh_lcd(void){
    void vPrintTesto(unsigned int uiRiga,unsigned int uiColonna, unsigned char xdata *pTesto);
    void vRefreshScreen(void);
    unsigned int ui_enable_external_ram(unsigned int);
    char msg[64];
    static unsigned char c;
    static int icnt,icnterr;
    // inizializzazione lcd
    vLCD_Init();
    // inizializzazione lista elementi display
    vInitListDisplayElements();   
    ui_enable_external_ram(0);
    Outputs_Hi=0;
    Outputs_Low=0;
    icnt=0;
    icnterr=0;

    while(1){
        Outputs_Hi|=0x80;
        c=Outputs_Hi;
        if (c!=0x80){
            icnterr++;
        }
    
        sprintf(msg,"hello: %i; err=%i",icnt++, icnterr);
        vPrintTesto(20,10, msg);
        vRefreshScreen();    
        c=Outputs_Hi;
        if (c!=0x80){
            icnterr++;
        }
        Outputs_Hi^=0x80;
        c=Outputs_Hi;
        if (c!=0x00){
            icnterr++;
        }
    }
}
#endif



int main(void) __attribute__((section("MAIN_ENTRY_POINT"),used,externally_visible));

// entry point del programma user
int main(void){


	extern void spi_main(void);
	extern void initTimer(void);
	extern void vInitializeTheI2c(void);
    extern void v_init_eeprom(void);
    extern void TargetResetInit(void);
// test che avevo usato per capire come mai sd non funzionava: il problema era nel modulo mci.c, il comando cmd0
// non vuole risposta, ma dopo averlo generato si deve attendere almeno 8 clock altrimenti se viene spedito un comando prima degli 8 clock
// sd card va in tilt...
//#define enable_disk_test
#ifdef enable_disk_test
    v_disk_test();
#endif


#if 0
    defIENABLE_nestedInts;
//    defIDISABLE_nestedInts;
#else
	defIDISABLE_nestedInts;
   // #warning ******  NESTED INTS ARE DISABLED!!! ******
#endif

    disableIRQ();
    // never enable interrupts here, else the system hangs! you should before set the new int vectors
    //enableIRQ();
	vAZ4186_InitHwStuff();
    v_init_eeprom();

// test lettura ds3231 rtc
#ifdef def_test_rtc_ds3231
    v_test_read_rtc_ds3231();
#endif


#ifdef defTestLcd
	vTestLcd();
#endif
#ifdef defTestOutputs
	vTestOutputs();
#endif
#ifdef defTestDac
	vTestDac();
#endif
#ifdef defTestAdc
    {
        extern void v_test_adc(void);
        v_test_adc();
    }
#endif


	// inizializzo i2c bus in modo da impostare a velocit� 0 il comando del coltello, per evitare che parta veloce all'accensione
	// della macchina
	// pertanto lo faccio immediatamente, ancora prima della attesa fpga
	vInitI2c();
	vInitializeTheDacValues();
#ifdef def_canopen_enable
    ui_canopen_init();
#endif
    
// test max519 AFTER i2c initialization!
#ifdef defTestMax519
    vTestMax519();
#endif
	// indico che la scheda sta facendo il boot, pertanto la routine di interrupt non deve
	// fare nulla se non aggiornare il contatore delle interrupt
	ucBooting=1;
#ifdef def_test_outdig_glitch    
    v_test_refresh_lcd();
#endif    
	// faccio partire la routine di interrupt timer0
	initTimer();
    
	// attesa boot fpga: serve per evitare di spedire comandi ad fpga mentre sta facendo il boot
	vWaitBootFpga();
    ui_enable_external_ram(1);
// resets/inits the wheel rotation
    {
        extern void v_init_rotation_wheel_inversion(void);
        v_init_rotation_wheel_inversion();
    }
    
    
	// chiamo il programma main di gestione spiralatrice
	// teoricamente non si esce mai dalla routine spi_main
	spi_main();
	// chiusura lcd, qui non si arriva mai, a meno di reboot
	//vCloseLCD();
    return 1;
}//void main(void)
