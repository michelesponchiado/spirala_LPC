#ifndef def_az4186_spiralatrice_h_included
#define def_az4186_spiralatrice_h_included

#include <stdint.h>
#include "az4186_const.h"


// enum of potentiometers handling 
typedef enum{noSlowRpmMan=0,
            slowRpmManPrg,
			slowRpmManCom,
			slowRpmManAll
	    }TipoSlowRpmMan;

typedef enum
{
	enum_stop_spindle_status_idle = 0,
	enum_stop_spindle_status_init,
	enum_stop_spindle_status_slow_down,
	enum_stop_spindle_status_ends,
	enum_stop_spindle_status_numof
}enum_stop_spindle_status;

#define def_min_stop_ramp_max_length_ms 300
#define def_max_stop_ramp_max_length_ms 2000
#define default_stop_ramp_max_length_ms 1000

#define def_period_update_stop_ramp_ms 50
#define def_enable_spindle_stop_ramp_down

typedef struct _type_stop_spindle
{
	unsigned int num_req;
	unsigned int num_ack;
	enum_stop_spindle_status status;
	unsigned int stop_ramp_max_length_ms; // the stop ramp length when the spindle DAC is full speed
	int num_step_decrease_per_period[3];
	unsigned int final_num_periods;
	unsigned int current_num_periods;
	unsigned long ul_prev_timer_interrupt_count;
	unsigned char ucDacOutValues[8];
}type_stop_spindle;

typedef enum
{
	enum_status_check_tangled_wire_idle = 0,
	enum_status_check_tangled_wire_wait_lock,
	enum_status_check_tangled_wire_check,
	enum_status_check_tangled_wire_validate,
	enum_status_check_tangled_wire_alarm,
	enum_status_check_tangled_wire_numof
}enum_status_check_tangled_wire;

#define def_period_check_tangled_wire_ms 100

typedef struct _type_check_tangled_wire
{
	enum_status_check_tangled_wire status;
	unsigned int speed_percentage; // a percentage of 0 means check disabled
	unsigned int min_duration_ms;
	unsigned int cur_duration_ms;
	float inv_vel_prod_to_check;
	float vel_prod_to_check;
	unsigned long ul_prev_timer_interrupt_count;
}type_check_tangled_wire;

typedef struct _TipoStruct_Spiralatrice{
	/* Numero del programma col primo codice. */
	// extern tipoNumeroProgramma xdat firstPrg;
	/* Numero del prossimo programma da inserire. */
	// extern tipoNumeroProgramma xdat nextPrgNum;
	/* Numero d' ordine della prima commessa. */
	unsigned char firstComm;
	/* Numero dell' ultima commessa inserita. */
	unsigned int  LastNumCommessa;
	COMMESSA  * pComm;
	COMMESSA  * pCommRun;
	PROGRAMMA * pPrg;
	PROGRAMMA * pPrgRun;
	/* Numero attuale di commessa. */
	unsigned char actCommessa;
	unsigned char runningCommessa;
	/* Posizione del coltello. */
	float actPosDacColtello;
	/* Corsa del coltello. */
	float CorsaColtello;
	/* Valori finali delle rampe dei dac che necessitano di rampa:
	   motori mandrino e frizioni.
	   Uso degli integer per evitare overflow nei calcoli delle ampiezze dei
	   gradini.
	*/
	unsigned int DacOut[NumDacRampa*2+2];
	unsigned char CheckPotMan,CheckPotVel[NUM_FRIZIONI];

	/* Valori da spedire in uscita ai dac. */
	unsigned long DacOutValue[2];
	unsigned long DacOutValue_dacstep[2];
	unsigned int DacOutOld[NumDacRampa*2];
	/* Massimo valore verso i dac. */
	unsigned long DacOutMaxValue[2];
	unsigned char ucAlarmSlowRpm;
	TipoSlowRpmMan slowRpmMan;
	/* Carattere che contiene i bit che identificano se un particolare
	   Dac necessita di variazione a gradino delle proprie uscite.
	   La maschera � fornita dall' indirizzo di dac.
	   bit 0 --> mandrino
	   bit 1 --> frizione 1
	   bit 2 --> frizione 2
	*/
	unsigned char StatusRampaDac;
	/* Somma dei raggi di coltello, mandrino e filo meno interasse
	   mandrino-coltello. */
	float SommaRaggiMenoInterasse;
	/* Numero del gradino della rampa da generare. */
	unsigned char GradinoRampaDAC;
	/* Indicatore del fatto che il primo pezzo prodotto dalla macchina
	   debba essere eliminato. */
	unsigned char PrimoPezzoEliminato;
	unsigned char nextRunCommessa;
	/* Char che indica se il valore della resistenza statica � valido o no. */
	unsigned char misStaOk;

	unsigned long TempoRefreshRampa;
	unsigned long TempoTaglioPezzo;
	unsigned long spaziorv;
	unsigned long temporv;
	unsigned int NumPezziScarto;
	/* Numero di pezzi fatti prima della partenza di una commessa. */
	unsigned int oldndo;


	/* Vale 1 se i potenziometri hanno il lucchetto, 0 altrimenti. */
	unsigned char AbilitaModificaPotenziometri;

	/* Backup valore degli allarmi. */
	uint32_t oldalarms;
	/* Timer della durata dell' allarme. */
	unsigned long TempoAllarme;
	unsigned char nextRunPrg;
	unsigned char OkMisStatica;
	/* Indica se la velocita' di una commessa deve essere clippata al valore
	   massimo indicato a programma. */
	unsigned char ucClipVelocity;
	/* Valore mediato della misura dall' ohmetro statico. */
	float resStaMediata,resStaMediataAux;
	float oldresStaMediata;
	unsigned int nSampleResSta;
	unsigned char OkAbilitaTaraturaStat;

	/* Indica se e' abilitata la taratura "fine" (i.e. prendendo le estremita'
	   della spirale) mediante ohmetro statico.
	*/
	unsigned char OkAbilitaTaraFineStat;
	unsigned char actFornitore;
	/* Valore del prossimo canale da leggere per aggiornare gli
	   ingressi analogici. */
	unsigned char numCanaleUpdate;
	unsigned char misUpdateStatus;
	unsigned char misUpdateSleep;
	/* Valore della misura letta dall' adc per agg.re gli ingressi analogici. */
	unsigned long  misUpdate;

	/* Numero di misure effettuate dai canali che danno l' assorbimento
	   delle ruote di contrasto. */
	char nMisAssorb;
	/* Appoggio per il calcolo della media degli assorbimenti
	   delle ruote di contrasto. */
	long int auxMediaCanaleAssorb;
	unsigned char TaraPortate;
	unsigned char ChangePortata;
	unsigned char actTaraPortata;
	unsigned char actCanPortata;
	unsigned char actPosLama;
	unsigned char StartPortata;
	float f_pos_coltello_spaziatura_mm;
	float f_raggio_coltello_meno_interasse;

	type_stop_spindle stop_spindle;
	type_check_tangled_wire check_tangled_wire;

}TipoStruct_Spiralatrice;

extern TipoStruct_Spiralatrice spiralatrice;

#endif //#ifndef def_az4186_spiralatrice_h_included
