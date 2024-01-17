#pragma once
#include "stddef.h"
#include "dev.h"

class Radio : public Dev
{
private:
    uint32_t m_power_dbm;
    uint32_t m_freq;
    uint32_t m_vpd_set;
    void*    m_mod_handle;
    void*    m_pa_handle;

private:
    uint16_t bilinearInterpolation(float dbm);

public:
    enum DriverId
    {
        RTC6705_RFPA5542,
    };
    enum State
    {
        READY = 0,
        BUSY,
    };

public:
    Radio(){};
    virtual int  open(DEV_ID id) ;
    virtual void close(void)     ;
    virtual void event_poll(void);
    int 		set_freq				(uint32_t freq) 		 		;
    int 		set_power				(uint32_t dbm) 			 		;
    int 		set_power_mw			(uint32_t mW)					;	
    int 		set_freq_power			(uint32_t freq, uint32_t power) ;
    State 		get_state				(void) 					const	;
    void 		get_freq_range			(uint32_t& min, uint32_t& max)	const;
    uint32_t 	get_max_power			(void)							const;
    void 		print_chip_regs			(void)							;

private:	// forbidden copy constructor
    Radio(const Radio&){}
};
