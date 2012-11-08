/* $Author: kalt_r $
*  $Date: 2012/11/08 12:35:27 $
* $Revision: 1.1 $
*
* ATTENTION:
* To be used only for Linux / embeddedlinux-e500v2.
*/
#define DEBUG 0

/* these two had to be defined since missing in .h files. Where to find them?*/
#define ERROR -1
#define OK 0

/* EPICS */
#include <genSubRecord.h>
#include <menuFtype.h>

/* Linux */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>


    
/**
 * Global structure for storing information by different tasks.
 */

typedef struct
{
    long measuringStarted;
    unsigned long startTimeCounter;
    long restartCounter;
    long restopCounter;
    long firstMeasuring;
    float timeMin;
    float timeMax;
} timeMeasureStorageData;

/** Initializes and spanws a timeMeasure task.
  * A    : Measuriung Start Stop indicator ( long ), 1 active
  * B    : Measuring reset indicator ( long ), 1 active
  * VALA : Actual time measured ( float ) / us
  * VALB : Minimum time measured ( float ) / us
  * VALC : Maximum time measured ( float ) / us
  * VALD : restart counter ( long )
  * VALE : restop counter ( long )
  * VALF : gettimeofday CPU-TIME us value
  * @param pointer to genSubRecord from epics
  * @return OK: Sub Task successfully created, ERROR when an error occurred while spawning Sub Task
  */
int LLRF_timeMeasureInit ( struct genSubRecord* genSubData )
{    
    timeMeasureStorageData* dataStorage = NULL;
    int init_error_flag=0;

    if ( DEBUG )
        printf ( " ( i: %s ) %s started.\r\n", __FILE__ + 3, __FUNCTION__ );
    if ( genSubData == NULL )
    {
        printf ( " ( e: %s: %s() line: %d ) No genSubData to process.\r\n", __FILE__ + 3, __FUNCTION__, __LINE__ );
        printf ( " ( i: %s: %s() ) Errno String: %s\r\n", __FILE__ + 3, __FUNCTION__, strerror ( errno ) );
        return ERROR;
    }
    if ( ( dataStorage = ( timeMeasureStorageData* ) malloc ( sizeof ( timeMeasureStorageData ) ) ) == NULL )
    {
        printf ( " ( e: %s: %s() line: %d ) malloc () returned NULL.\r\n", __FILE__ + 3, __FUNCTION__, __LINE__ );
        printf ( " ( i: %s: %s() ) Errno String: %s\r\n", __FILE__ + 3, __FUNCTION__, strerror ( errno ) );
        return ERROR;
    }
    genSubData -> dpvt = dataStorage;
    dataStorage -> measuringStarted = 0;
    dataStorage -> startTimeCounter = 0;
    dataStorage -> restartCounter = 0;
    dataStorage -> restopCounter = 0;
    dataStorage -> firstMeasuring = 1;
    dataStorage -> timeMin = 0.0;
    dataStorage -> timeMax = 0.0;
    {/*Checks for input output fields*/
        init_error_flag = 0;
        if (genSubData->fta!=menuFtypeLONG) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) FTA not LONG\r\n", __FUNCTION__);
        }
        if (genSubData->noa!=1)
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) NOA not 1\r\n", __FUNCTION__);
        }
        if (genSubData->ftb!=menuFtypeLONG) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) FTB not LONG\r\n", __FUNCTION__);
        }
        if (genSubData->nob!=1)
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) NOB not 1\r\n", __FUNCTION__);
        }
        if (genSubData->ftva!=menuFtypeFLOAT) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) FTVA type not FLOAT\r\n", __FUNCTION__);
        }
        if (genSubData->nova!=1) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) NOVA not 1\r\n", __FUNCTION__);
        }
        if (genSubData->ftvb!=menuFtypeFLOAT) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) FTVB type not FLOAT\r\n", __FUNCTION__);
        }
        if (genSubData->novb!=1) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) NOVB not 1\r\n", __FUNCTION__);
        }
        if (genSubData->ftvc!=menuFtypeFLOAT) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) FTVC type not FLOAT\r\n", __FUNCTION__);
        }
        if (genSubData->novc!=1) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) NOVC not 1\r\n", __FUNCTION__);
        }
        if (genSubData->ftvd!=menuFtypeLONG) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) FTVD type not LONG\r\n", __FUNCTION__);
        }
        if (genSubData->novd!=1) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) NOVD not 1\r\n", __FUNCTION__);
        }
        if (genSubData->ftve!=menuFtypeLONG) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) FTVE type not LONG\r\n", __FUNCTION__);
        }
        if (genSubData->nove!=1) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) NOVE not 1\r\n", __FUNCTION__);
        }
        if (genSubData->ftvf!=menuFtypeFLOAT) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) FTVF type not FLOAT\r\n", __FUNCTION__);
        }
        if (genSubData->novf!=1) 
        {
            init_error_flag=ERROR;
            printf(" ( e: %s ) NOVF not 1\r\n", __FUNCTION__);
        }
        if (init_error_flag==ERROR)
        {
            printf ( " ( e: %s: %s() line: %d ) Aborted.\r\n", __FILE__ + 3, __FUNCTION__, __LINE__ );
            return ERROR;
        }
    }
    if ( DEBUG )
        printf ( " ( i: %s ) %s initialized completely.\r\n", __FILE__ + 3, __FUNCTION__ );
    return OK;
}

/** Function is called from genSubRecord when PROC filed is set to 1. 
  * The funtion stores the actual time parameters when inpa = 1 given in the genSubData. And returns the time difference when called with inpa = 0.
  * @param genSubData genSubRecord from epics
  * @return ERROR or OK
  */
int LLRF_timeMeasure ( struct genSubRecord* genSubData )
{
    timeMeasureStorageData* dataStorage = NULL;
    struct timeval tv1;
    long startStopSelect = 0;
    long reset = 0;
    float *outTimeAct = NULL;
    float *outTimeMin = NULL;
    float *outTimeMax = NULL;
    long *outRestartCounter = NULL;
    long *outRestopCounter = NULL;
    float *cpuTime = NULL;
    
    if ( DEBUG )
        printf ( " ( i: %s ) %s started.\r\n", __FILE__ + 3, __FUNCTION__ );
    if ( genSubData == NULL )
    {
        printf ( " ( e: %s: %s() ) No genSubData to process\r\n", __FILE__ + 3, __FUNCTION__ );
        return ERROR;
    }
    dataStorage = ( timeMeasureStorageData* ) genSubData -> dpvt;
    if ( dataStorage == NULL )
    {
        printf ( " ( e: %s: %s() ) genSubData -> dpvt = NULL. Seams that the init function was not called yet, or had errors. Aborting.\r\n", __FILE__ + 3, __FUNCTION__ );
        return ERROR;
    }

    startStopSelect         = *( long * ) genSubData -> a;
    reset                   = *( long * ) genSubData -> b;
    outTimeAct              = ( float * ) genSubData -> vala;
    outTimeMin              = ( float * ) genSubData -> valb;
    outTimeMax              = ( float * ) genSubData -> valc;
    outRestartCounter       = ( long * )  genSubData -> vald;
    outRestopCounter        = ( long * )  genSubData -> vale;
    cpuTime                 =             genSubData -> valf;
    
    gettimeofday(&tv1,NULL);
    * cpuTime = (float) tv1.tv_usec;
    /*vxTimeBaseGet (& pTbu, & pTbl);
    * cpuTime = ( ( float ) pTbl ) / 33333333;*/
    
    if ( reset )
        dataStorage -> firstMeasuring = 1;
    if ( startStopSelect == 1 && dataStorage -> measuringStarted == 0 )
    {
        dataStorage -> startTimeCounter = (float) tv1.tv_usec;
        dataStorage -> measuringStarted = 1;
    }
    else if ( startStopSelect == 1 && dataStorage -> measuringStarted == 1 )
    {
        dataStorage -> restartCounter ++; /* counts up serveral start commands */
        *outRestartCounter = dataStorage -> restartCounter;
    }
    else if ( startStopSelect == 0 && dataStorage -> measuringStarted == 0 )
    {
        dataStorage -> restopCounter ++; /* counts up serveral start commands */
        *outRestopCounter = dataStorage -> restopCounter;
    }
    else if ( startStopSelect == 0 && dataStorage -> measuringStarted == 1 )
    {
        dataStorage -> measuringStarted = 0;
        
        *outTimeAct = ( ( float ) ( (float)(tv1.tv_usec) - dataStorage -> startTimeCounter ) );
        /**outTimeAct = ( ( float ) ( pTbl - dataStorage -> startTimeCounter ) ) / 33333.333;*/
        
        if ( dataStorage -> firstMeasuring )
        {
            dataStorage -> timeMax = * outTimeAct;
            *outTimeMax = *outTimeAct;
            dataStorage -> timeMin = * outTimeAct;
            *outTimeMin = *outTimeAct;
            dataStorage -> restartCounter = 0;
            *outRestartCounter = 0;
            dataStorage -> restopCounter = 0;
            *outRestopCounter = 0;
            dataStorage -> firstMeasuring = 0;
        }
        if ( *outTimeAct > dataStorage -> timeMax )
        {
            dataStorage -> timeMax = * outTimeAct;
            *outTimeMax = *outTimeAct;
        }
        if ( *outTimeAct < dataStorage -> timeMin )
        {
            dataStorage -> timeMin = * outTimeAct;
            *outTimeMin = *outTimeAct;
        }
    }
    else 
        if ( DEBUG )
            printf ( " ( w: %s ) %s Stop without Start event.\r\n", __FILE__ + 3, __FUNCTION__ );
    if ( DEBUG )
        printf ( " ( i: %s ) %s done.\r\n", __FILE__ + 3, __FUNCTION__ );
    return OK;
}
