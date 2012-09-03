/*$Name:  $*/
/*$Author: kalantari $*/
/*$Date: 2012/09/03 08:15:53 $*/
/*$Revision: 1.7 $*/
/*$Source: /cvs/G/DRV/pev/ifcDev.c,v $*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pevulib.h>

#include <epicsTypes.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recSup.h>
#include <recGbl.h>
#include <link.h>
#include <alarm.h>
#include <dbScan.h>
#include <dbAccess.h>

#define BIT_31_SET  0x80000000

long  ifc1210Init(){ return 0; }
struct {
    long number;
    long (*report) ();
    long (*init) ();
} drvIfc1210 = {
    2,
    NULL,
    ifc1210Init
};


typedef enum { IFC_ELB, IFC_SMON, PCI_IO, BMR, BMR_11U, BMR_11S, BMR_16U } IfcDevType;

typedef struct ifcPrivate{
    unsigned int address;
    IfcDevType devType;
    unsigned int card;
    unsigned int count;
} ifcPrivate;

static long devIfc1210InitRecord(dbCommon* record, struct link* link)
{
    ifcPrivate* p;
     
    if (link->type != VME_IO)
    {
        recGblRecordError(S_db_badField, record,
            "ifc: Wrong type of io link");
        return S_db_badField;
    }
    if ((p = malloc(sizeof(ifcPrivate))) == NULL)
    {
        recGblRecordError(errno, record,
            "devIfc1210InitRecord: Out of memory");
         return errno;
    }
    p->address = link->value.vmeio.signal;
    p->card  = link->value.vmeio.card;;
    
    if(strcmp(link->value.vmeio.parm, "ELB") == 0) 
    	p->devType = IFC_ELB;
    else 
    if(strcmp(link->value.vmeio.parm, "SMON") == 0) 
    	p->devType = IFC_SMON;
    else 
    if(strcmp(link->value.vmeio.parm, "PIO") == 0) 
    	p->devType = PCI_IO;
    else 
    if(strncmp(link->value.vmeio.parm, "BMR_11U", 7) == 0) 
      { 
	  p->devType = BMR_11U;
  	  p->count = atoi(strchr( link->value.vmeio.parm, ' ')+1);
      }
    else 
    if(strncmp(link->value.vmeio.parm, "BMR_11S", 7) == 0) 
      { 
    	  p->devType = BMR_11S;
	  p->count = atoi(strchr( link->value.vmeio.parm, ' ')+1);
      }
    else 
    if(strncmp(link->value.vmeio.parm, "BMR_16U", 7) == 0) 
      { 
    	  p->devType = BMR_16U;
	  p->count = atoi(strchr( link->value.vmeio.parm, ' ')+1);
      }
    else 
    if(strncmp(link->value.vmeio.parm, "BMR", 3) == 0)
      { 
    	  p->devType = BMR;
	  p->count = atoi(strchr( link->value.vmeio.parm, ' ')+1);
      }
   else 
        {
        recGblRecordError(S_db_badField, record,
            "devIfc1210InitRecord: Illegal param in io link");
        return S_db_badField;
	}

    record->dpvt = p;
    return 0;
}

/*************** ai record ****************/

#include <aiRecord.h>


long devIfc1210AiInitRecord(aiRecord* record)
{
   ifcPrivate* p;
   int status=0;
   status = devIfc1210InitRecord((dbCommon*) record, &record->inp);
   if (status != 0) return status; 
   p = record->dpvt;
   record->udf = 0;
   return 0;
}

long devIfc1210AiRead(aiRecord* record)
{
   ifcPrivate* p = record->dpvt;
   int rval = 0;
    
   if (p == NULL)
    {
        recGblRecordError(S_db_badField, record,
            "devSigAnalyzerAiRead: uninitialized record");
        recGblSetSevr(record, UDF_ALARM, INVALID_ALARM);
        return -1;
    }
    
    if(p->devType == IFC_ELB)
    	rval = pev_elb_rd( p->address );
    else
    if(p->devType == IFC_SMON)
    	rval = pev_smon_rd( p->address );
    if(p->devType == PCI_IO)
    	rval = pev_csr_rd( p->address | 0x80000000 );
    else
    if(p->devType == BMR)
        rval = pev_bmr_read( p->card,  p->address, p->count);
    else
    if(p->devType == BMR_11U)
      {
        rval = pev_bmr_read( p->card,  p->address, p->count);
	record->val = pev_bmr_conv_11bit_u(rval);
        usleep( 10000);
	return 2;
      }
    else
    if(p->devType == BMR_11S)
      {
        rval = pev_bmr_read( p->card,  p->address, p->count);
	record->val = pev_bmr_conv_11bit_s(rval);
        usleep( 10000);
	return 2;
      }
    else
    if(p->devType == BMR_16U)
      {
        rval = pev_bmr_read( p->card,  p->address, p->count);
	record->val = pev_bmr_conv_16bit_u(rval);
        usleep( 10000);
	return 2;
      }
    
    record->val = rval;
    return 2; 	/* no conversion */
}

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devIfc1210Ai = {
    6,
    NULL,
    NULL,
    devIfc1210AiInitRecord,
    NULL,
    devIfc1210AiRead,
    NULL
};


/*************** ao record  ****************/



#include <aoRecord.h>


long devIfc1210AoInitRecord(aoRecord* record)
{
   int status=0;
   status = devIfc1210InitRecord((dbCommon*) record, &record->out);
   if (status != 0) return status; 
   record->udf = 0;
   return 0;
}

long devIfc1210AoWrite(aoRecord* record)
{
    ifcPrivate* p = record->dpvt;
    
   if (p == NULL)
    {
        recGblRecordError(S_db_badField, record,
            "devIfc1210AoAoWrite: uninitialized record");
        recGblSetSevr(record, UDF_ALARM, INVALID_ALARM);
        return -1;
    }

    if(p->devType == IFC_ELB)
    	pev_elb_wr( p->address, (int)record->val );
    else
    if(p->devType == IFC_SMON)
    	pev_smon_wr( p->address, (int)record->val );
    else
    if(p->devType == PCI_IO)
    	pev_csr_wr( p->address | 0x80000000, (unsigned int)record->val);
    else
    if(p->devType == BMR) 
    	pev_bmr_write(p->card, p->address, (unsigned int)record->val, p->count);
	
    return 0; 
}

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN write_ao;
    DEVSUPFUN special_linconv;
} devIfc1210Ao = {
    6,
    NULL,
    NULL,
    devIfc1210AoInitRecord,
    NULL,
    devIfc1210AoWrite,
    NULL
};
