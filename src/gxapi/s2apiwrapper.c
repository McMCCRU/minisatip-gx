/*
 * s2apiwrapper.c:
 * Wrapper to translate DVB S2API to DVB 3.0 API calls
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: $
 */

#include "s2apiwrapper.h"
#include "tools.h"
#include <string.h>
#include <errno.h>

#ifdef DVB_S2API_WRAPPER

static int ioctl_FE_SET_PROPERTY(int d, dtv_properties *properties) {
  int result = 0;
  fe_delivery_system Delivery = SYS_UNDEFINED;
  dvb_frontend_parameters Frontend;
  memset(&Frontend, 0, sizeof(Frontend));
  
  for (__u32 i=0; i<properties->num; i++) {
      __u32 cmd = properties->props[i].cmd;
      __u32 data = properties->props[i].u.data;
      
      switch (cmd) {
        case DTV_CLEAR: 
             memset(&Frontend, 0, sizeof(Frontend));
             continue;
                     
        case DTV_DELIVERY_SYSTEM:
             Delivery = fe_delivery_system(data);
             continue;
             
        case DTV_FREQUENCY:
             Frontend.frequency = data;
             continue;
             
        case DTV_INVERSION:
             Frontend.inversion = fe_spectral_inversion(data);
             continue;

        case DTV_TUNE:
             result = ioctl(d, FE_SET_FRONTEND, &Frontend);
             if (result < 0)
             	return result;
             continue;
        }

      if (Delivery == SYS_DVBS) {
         switch (cmd) {
           case DTV_MODULATION:
           	    if (data != FE_QPSK) {
                	errno = EINVAL;
                	return -1; 
                	}
                break;
                
           case DTV_SYMBOL_RATE:
                Frontend.u.qpsk.symbol_rate = data;
                break;
                
           case DTV_INNER_FEC:
                Frontend.u.qpsk.fec_inner = fe_code_rate(data);
                break;
                
           case DTV_ROLLOFF:
                if (data != ROLLOFF_35) { 
                	errno = EINVAL;
                	return -1; 
                	}
                break;
	       default:
	         	errno = EINVAL;
	            return -1; 
           }
         }
      else if (Delivery == SYS_DVBC_ANNEX_AC && result >= 0) {
         switch (cmd) {
           case DTV_MODULATION:
                Frontend.u.qam.modulation = fe_modulation(data);
                break;
                
           case DTV_SYMBOL_RATE:
                Frontend.u.qam.symbol_rate = data;
                break;
                
           case DTV_INNER_FEC:
                Frontend.u.qam.fec_inner = fe_code_rate(data);
                break;
	       default:
	         	errno = EINVAL;
	            return -1; 
           }
         }
      else if (Delivery == SYS_DVBT && result >= 0) {
         switch (cmd) {
           case DTV_MODULATION:
                Frontend.u.ofdm.constellation = fe_modulation(data);
                break;
                
           case DTV_BANDWIDTH_HZ:
                Frontend.u.ofdm.bandwidth = fe_bandwidth(data);
                break;
               
           case DTV_CODE_RATE_HP:
                Frontend.u.ofdm.code_rate_HP = fe_code_rate(data);
                break;
               
           case DTV_CODE_RATE_LP:
                Frontend.u.ofdm.code_rate_LP = fe_code_rate(data);
                break;
               
           case DTV_TRANSMISSION_MODE:
                Frontend.u.ofdm.transmission_mode = fe_transmit_mode(data);
                break;
               
           case DTV_GUARD_INTERVAL:
                Frontend.u.ofdm.guard_interval = fe_guard_interval(data);
                break;
                
           case DTV_HIERARCHY:
                Frontend.u.ofdm.hierarchy_information = fe_hierarchy(data);
                break;

	       default:
	         	errno = EINVAL;
	            return -1; 
           }
         }
      } // for
  return result;
}

static int ioctl_FE_GET_PROPERTY(int d, dtv_properties *properties) {
  errno = EINVAL;
  return -1;
}

#ifdef DVB_S2API_RUNTIME
static int DvbApiVersion = 0;

void S2API_SetDvbApiVersion(int ApiVersion) {
   isyslog("Using DVB API v%d", ApiVersion);
   DvbApiVersion = ApiVersion;
   }
#endif

int S2API_ioctl(int d, int request, void *data) {
  // Fast bypass for non-S2API call:
  if (request != (int)FE_SET_PROPERTY && request != (int)FE_GET_PROPERTY)
     return ioctl(d, request, data);

#ifdef DVB_S2API_RUNTIME
  // Fast bypass if we have S2API:
  if (DvbApiVersion >= 5) // Do we have native S2API?
     return ioctl(d, request, data);

  if (!DvbApiVersion) {
     // Try S2API native
     int err = ioctl(d, request, data);
     if (err != -1 || errno != EOPNOTSUPP) {
        // ok or other error: we have S2API
        isyslog("S2API support detected");
        S2API_SetDvbApiVersion(5);
        return err;
        }
     // error OPNOTSUPP: No S2API support.
     isyslog("No S2API support detected");
     S2API_SetDvbApiVersion(3);
  }
#endif // ifdef DVB_S2API_RUNTIME

  if (request == (int)FE_SET_PROPERTY)
     return ioctl_FE_SET_PROPERTY(d, (dtv_properties*)data);
  if (request == (int)FE_GET_PROPERTY)
     return ioctl_FE_GET_PROPERTY(d, (dtv_properties*)data);

  // never get here
  return ioctl(d, request, data);
}

#endif // ifdef DVB_S2API_WRAPPER
