#define    Event_Bytes        12023        //Size of event data in bytes
#define    ADC_Full_Scale_Low    2030        //ADC Full Scale low
#define    ADC_Full_Scale_High    2060        //ADC Full Scale high
#define    ADC_Off_Low        2047        //ADC Offset low
#define    ADC_Off_High        2049        //ADC Offset high
#define    ADC_Common_Off_Low    149        //ADC Common offset low (use for gain calib)
#define    ADC_Common_Off_High    151        //ADC Common offset high (use for gain calib)
#define    ADC_Gain_Low        149        //ADC Gain low
#define    ADC_Gain_High        151        //ADC Gain high
//Final common offset value is set such that the average ADC sum on each channel is within this range
#define    Final_Common_Off_Low    16        //Final common offset value
#define    Final_Common_Off_High    17        //Use for setting final common offset value

#define MSG_START    0x99 //!< start of fpga message
#define MSG_END      0x66 //!< end of fpga message


/* Time stamp offsets (within 7B time field) */

#define TIME_YEAR       0
#define TIME_MON        2
#define TIME_DAY        3
#define TIME_HOUR       4
#define TIME_MIN        5
#define TIME_SEC        6


/*----------------------------------------------------------------------*/
/* Maxima / minima */

/* Maximum ADC size = 4 channels * max_samples/ch * 2 bytes/sample */
/* Maximum event size = header + ADC data + message end       */

#define DATA_MAX_SAMP   4096                       //!< Maximal trace length (samples)
#define MAX_READOUT     (70 + DATA_MAX_SAMP*8 + 2) //!< Maximal raw event size

#define MIN_MSG_LEN     6                          //!< Minimal length of scope message

/*----------------------------------------------------------------------*/
/* Message definitions */
#define ID_PARAM_GPS          0x00
#define ID_PARAM_CTRL         0x01
#define ID_PARAM_WINDOWS      0x02
#define ID_PARAM_COMRES       0x03
#define ID_PARAM_SPI          0x04
#define ID_PARAM_TRIG5        0x05
#define ID_PARAM_CH1          0x08
#define ID_PARAM_CH2          0x09
#define ID_PARAM_CH3          0x0A
#define ID_PARAM_CH4          0x0B
#define ID_PARAM_TRIG1        0x0C
#define ID_PARAM_TRIG2        0x0D
#define ID_PARAM_TRIG3        0x0E
#define ID_PARAM_TRIG4        0x0F
#define ID_PARAM_FILT11       0x10
#define ID_PARAM_FILT12       0x11
#define ID_PARAM_FILT21       0x12
#define ID_PARAM_FILT22       0x13
#define ID_PARAM_FILT31       0x14
#define ID_PARAM_FILT32       0x15
#define ID_PARAM_FILT41       0x16
#define ID_PARAM_FILT42       0x17
#define ID_PARAM_PPS          0xC4
#define ID_PARAM_EVENT        0xC0
#define ID_PARAM_ERROR        0xCE
#define ID_PARAM_NOP          0x66
#define ID_PARAM_LORA          0x20

#define LEN_MODE_PARAM 0x09
#define LEN_READOUT_PARAM 0x0B
#define LEN_CH_PROPERTY_PARAM 0x09
#define LEN_CH_TRIGGER_PARAM 0x09



/* Error Definition */
#define ERROR_BCNT 2
#define ERROR_ID   4
#define ERROR_END  6

#define PARAM_NUM_LIST 0x18     //!< Number of parameter lists for the fpga
#define PARAM_LIST_MAXSIZE 22   //!< maximal listsize 22 bytes



uint8_t shadowlist[PARAM_NUM_LIST][PARAM_LIST_MAXSIZE];  //!< all parameters to set in FPGA
uint8_t shadowlistR[PARAM_NUM_LIST][PARAM_LIST_MAXSIZE]; //!< all parameters read from FPGA
//int32_t shadow_filled = 0;                               //!< the shadow list is not filled


uint16_t dig_mode_params[LEN_MODE_PARAM ];
uint16_t readout_window_params[LEN_READOUT_PARAM];
uint16_t ch_property_params[4][LEN_CH_PROPERTY_PARAM];
uint16_t ch_trigger_params[4][LEN_CH_TRIGGER_PARAM];







/*
char* Hostname="lora" ;
char* Master_Name="Master" ;
char* Master_Serial_No="FTQZTB7R" ;
int Master_Port_No=3301 ;
*/
