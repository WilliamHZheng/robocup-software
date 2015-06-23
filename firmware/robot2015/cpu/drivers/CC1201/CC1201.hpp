#pragma once

#include "mbed.h"
//#include "cmsis_os.h"
#include "CommLink.hpp"
//#include "RTP.hpp"
#include "CC1201Defines.hpp"

enum ext_flag_t { EXT_FLAG_OFF, EXT_FLAG_ON };

class CC1201 : public CommLink
{
	public:

		CC1201();
	
		CC1201(PinName mosi, PinName miso, PinName sck, PinName cs, PinName intPin = NC);


		virtual ~CC1201();

		virtual int32_t sendData(uint8_t* buffer, uint8_t size);

		virtual int32_t getData(uint8_t*, uint8_t*);

		virtual void reset(void);
    
		virtual int32_t selfTest(void);
		    
		virtual bool isConnected(void);


		void powerOnReset(void);

		//void ready(void);

		uint8_t mode(void);

		uint8_t status(void);


		// TODO: Move any direct register reads/writes & strobes to protected when done testing
		uint8_t strobe(uint8_t addr);

		uint8_t readReg(uint8_t addr, bool ext_flag_t = EXT_FLAG_OFF);

		uint8_t readReg(uint8_t addr, uint8_t* buffer, uint8_t len, bool ext_flag_t = EXT_FLAG_OFF);

		uint8_t writeReg(uint8_t addr, uint8_t value, bool ext_flag_t = EXT_FLAG_OFF);

		uint8_t writeReg(uint8_t addr, uint8_t* buffer, uint8_t len, bool ext_flag_t = EXT_FLAG_OFF);

	    // Send the command strobe to flush the TX or RX buffers on the CC1201
	    void flush_tx(void);
	    void flush_rx(void);
	    void calibrate(void);
	    void update_rssi(void);
	    float rssi(void);
	    uint8_t idle(void);
	    uint8_t rand(void);
	    uint8_t freq_update(void);
	    float frequency(void);
	    bool isLocked(void);

	protected:


	private:
		
		uint8_t status(uint8_t addr);

		uint8_t readRegExt(uint8_t addr);

		uint8_t readRegExt(uint8_t addr, uint8_t* buffer, uint8_t len);

		uint8_t writeRegExt(uint8_t addr, uint8_t value);

		uint8_t writeRegExt(uint8_t addr, uint8_t* buffer, uint8_t len);

		uint8_t     _lqi;
    	uint8_t     _chip_version;

		bool _isInit;
		float _rssi;
};

