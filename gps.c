#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "gps.h"
#include "rui.h"
#include "bsp.h"
extern RUI_GPIO_ST Gps_Power_Ctl;

#define TRIGGER_GPS_CNT                             10
/*!
 * \brief Buffer holding the  raw data received from the gps
 */
uint8_t NmeaString[128];

/*!
 * \brief Maximum number of data byte that we will accept from the GPS
 */
uint8_t NmeaStringSize = 0;

/* Various type of NMEA data we can receive with the Gps */
const char NmeaDataTypeGPGGA[] = "GPGGA";
const char NmeaDataTypeGPGSA[] = "GPGSA";
const char NmeaDataTypeGPGSV[] = "GPGSV";
const char NmeaDataTypeGPRMC[] = "GPRMC";

/* Value used for the conversion of the position from DMS to decimal */
const int32_t MaxNorthPosition = 8388607;       // 2^23 - 1
const int32_t MaxSouthPosition = 8388608;       // -2^23
const int32_t MaxEastPosition = 8388607;        // 2^23 - 1    
const int32_t MaxWestPosition = 8388608;        // -2^23

tNmeaGpsData NmeaGpsData;

bool HasFix = false;
static double Latitude = 0;
static double Longitude = 0;

static int32_t LatitudeBinary = 0;
static int32_t LongitudeBinary = 0;

static int32_t Altitude = 0xFFFFFFFF;

static uint32_t PpsCnt = 0;

bool PpsDetected = false;

void GpsPpsHandler( bool *parseData )
{
    PpsDetected = true;
    PpsCnt++;
    *parseData = false;

    if( PpsCnt >= TRIGGER_GPS_CNT )
    {   
        PpsCnt = 0;
        BlockLowPowerDuringTask ( true );
        *parseData = true;
    }
}
#define GPS_POWER_EN                            14
extern user_store_data_t user_store_data;
void GpsInit( void )
{
    PpsDetected = false;
    Gps_Power_Ctl.pin_num = GPS_POWER_EN;
    Gps_Power_Ctl.dir = RUI_GPIO_PIN_DIR_OUTPUT;
    Gps_Power_Ctl.pull = RUI_GPIO_PIN_NOPULL;
    rui_gpio_init(&Gps_Power_Ctl);
    GpsStop();    

    rui_uart_init(RUI_UART3,BAUDRATE_9600);

    GpsStart();

    RUI_LOG_PRINTF("GPS Init OK.GPS timeout:%ds\r\n",user_store_data.gps_timeout_cnt);

}



void SetFlightMode( void )
{
  // Send navigation configuration command
  uint8_t setNav[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC};


  rui_uart_send(RUI_UART3, &setNav, sizeof(setNav));
}

void GpsStart( void )
{
    GpsResetPosition();
    HasFix = false;
    rui_gpio_rw(RUI_IF_WRITE,&Gps_Power_Ctl,high);

    rui_delay_ms(300);
    RUI_LOG_PRINTF("Configuring GPS into Flight Mode.\r\n");
    SetFlightMode();
    RUI_LOG_PRINTF("Done.\r\n");

}

void GpsStop( void )
{
    rui_gpio_rw(RUI_IF_WRITE,&Gps_Power_Ctl,low);
}


bool GpsHasFix( void )
{
    return HasFix;
}

void GpsConvertPositionIntoBinary( void )
{
    long double temp;

    if( Latitude >= 0 ) // North
    {    
        temp = Latitude * MaxNorthPosition;
        LatitudeBinary = temp / 90;
    }
    else                // South
    {    
        temp = Latitude * MaxSouthPosition;
        LatitudeBinary = temp / 90;
    }

    if( Longitude >= 0 ) // East
    {    
        temp = Longitude * MaxEastPosition;
        LongitudeBinary = temp / 180;
    }
    else                // West
    {    
        temp = Longitude * MaxWestPosition;
        LongitudeBinary = temp / 180;
    }
}

void GpsConvertPositionFromStringToNumerical( void )
{
    int i;

    double valueTmp1;
    double valueTmp2;
    double valueTmp3;
    double valueTmp4;

    // Convert the latitude from ASCII to uint8_t values
    for( i = 0 ; i < 10 ; i++ )
    {
        NmeaGpsData.NmeaLatitude[i] = NmeaGpsData.NmeaLatitude[i] & 0xF;
    }
    // Convert latitude from degree/minute/second (DMS) format into decimal
    valueTmp1 = ( double )NmeaGpsData.NmeaLatitude[0] * 10.0 + ( double )NmeaGpsData.NmeaLatitude[1];
    valueTmp2 = ( double )NmeaGpsData.NmeaLatitude[2] * 10.0 + ( double )NmeaGpsData.NmeaLatitude[3];
    valueTmp3 = ( double )NmeaGpsData.NmeaLatitude[5] * 1000.0 + ( double )NmeaGpsData.NmeaLatitude[6] * 100.0 + 
                ( double )NmeaGpsData.NmeaLatitude[7] * 10.0 + ( double )NmeaGpsData.NmeaLatitude[8];
                
    Latitude = valueTmp1 + ( ( valueTmp2 + ( valueTmp3 * 0.0001 ) ) / 60.0 );

    if( NmeaGpsData.NmeaLatitudePole[0] == 'S' )
    {
        Latitude *= -1;
    }
 
    // Convert the longitude from ASCII to uint8_t values
    for( i = 0 ; i < 10 ; i++ )
    {
        NmeaGpsData.NmeaLongitude[i] = NmeaGpsData.NmeaLongitude[i] & 0xF;
    }
    // Convert longitude from degree/minute/second (DMS) format into decimal
    valueTmp1 = ( double )NmeaGpsData.NmeaLongitude[0] * 100.0 + ( double )NmeaGpsData.NmeaLongitude[1] * 10.0 + ( double )NmeaGpsData.NmeaLongitude[2];
    valueTmp2 = ( double )NmeaGpsData.NmeaLongitude[3] * 10.0 + ( double )NmeaGpsData.NmeaLongitude[4];
    valueTmp3 = ( double )NmeaGpsData.NmeaLongitude[6] * 1000.0 + ( double )NmeaGpsData.NmeaLongitude[7] * 100;
    valueTmp4 = ( double )NmeaGpsData.NmeaLongitude[8] * 10.0 + ( double )NmeaGpsData.NmeaLongitude[9];

    Longitude = valueTmp1 + ( valueTmp2 / 60.0 ) + ( ( ( valueTmp3 + valueTmp4 ) * 0.0001 ) / 60.0 );

    if( NmeaGpsData.NmeaLongitudePole[0] == 'W' )
    {
        Longitude *= -1;
    }
}
void GpsConvertAltitudeFromStringToNumerical( void )
{
    Altitude = atoi( NmeaGpsData.NmeaAltitude ); 
}

uint8_t GpsGetLatestGpsPositionDouble( double *lati, double *longi )
{
    uint8_t status = FAIL;
    if( HasFix == true )
    {
        status = SUCCESS;
    }
    else
    {
        GpsResetPosition( );
    }
    *lati = Latitude;
    *longi = Longitude;
    return status;
}

uint8_t GpsGetLatestGpsPositionBinary( int32_t *latiBin, int32_t *longiBin )
{
    uint8_t status = FAIL;

    BoardDisableIrq( );
    if( HasFix == true )
    {
        status = SUCCESS;
    }
    else
    {
        GpsResetPosition( );
    }
    *latiBin = LatitudeBinary;
    *longiBin = LongitudeBinary;
    BoardEnableIrq( );
    return status;
}

uint8_t GpsGetLatestGpsAltitude( int32_t* alt )
{
    uint8_t status = FAIL;
    if( HasFix == true )
    {
        status = SUCCESS;
    }
    else
    {
        Altitude = 0xFFFFFFFF;
    }
    * alt = Altitude;
    return status;
}

/*!
 * Calculates the checksum for a NMEA sentence
 *
 * Skip the first '$' if necessary and calculate checksum until '*' character is
 * reached (or buffSize exceeded).
 *
 * \retval chkPosIdx Position of the checksum in the sentence
 */
int32_t GpsNmeaChecksum( int8_t *nmeaStr, int32_t nmeaStrSize, int8_t * checksum )
{
    int i = 0;
    uint8_t checkNum = 0;

    // Check input parameters
    if( ( nmeaStr == NULL ) || ( checksum == NULL ) || ( nmeaStrSize <= 1 ) )
    {
        return -1;
    }

    // Skip the first '$' if necessary
    if( nmeaStr[i] == '$' )
    {
        i += 1;
    }

    // XOR until '*' or max length is reached
    while( nmeaStr[i] != '*' )
    {
        checkNum ^= nmeaStr[i];
        i += 1;
        if( i >= nmeaStrSize )
        {
            return -1;
        }
    }

    // Convert checksum value to 2 hexadecimal characters
    checksum[0] = Nibble2HexChar( checkNum / 16 ); // upper nibble
    checksum[1] = Nibble2HexChar( checkNum % 16 ); // lower nibble

    return i + 1;
}

/*!
 * Calculate the checksum of a NMEA frame and compare it to the checksum that is
 * present at the end of it.
 * Return true if it matches
 */
static bool GpsNmeaValidateChecksum( int8_t *serialBuff, int32_t buffSize )
{
    int32_t checksumIndex;
    int8_t checksum[2]; // 2 characters to calculate NMEA checksum

    checksumIndex = GpsNmeaChecksum( serialBuff, buffSize, checksum );

    // could we calculate a verification checksum ?
    if( checksumIndex < 0 )
    {
        return false;
    }

    // check if there are enough char in the serial buffer to read checksum
    if( checksumIndex >= ( buffSize - 2 ) )
    {
        return false;
    }

    // check the checksum
    if( ( serialBuff[checksumIndex] == checksum[0] ) && ( serialBuff[checksumIndex + 1] == checksum[1] ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint8_t GpsParseGpsData( int8_t *rxBuffer, int32_t rxBufferSize )
{
    uint8_t i = 1;
    uint8_t j = 0;
    uint8_t fieldSize = 0;
  
    if( rxBuffer[0] != '$' )
    {
        return FAIL;
    }

    if( GpsNmeaValidateChecksum( rxBuffer, rxBufferSize ) == false )
    {
        return FAIL;
    }

    fieldSize = 0;
    while( rxBuffer[i + fieldSize++] != ',' )
    {
        if( fieldSize > 6 )
        {
            return FAIL;
        }
    }
    for( j = 0; j < fieldSize; j++, i++ )
    {
        NmeaGpsData.NmeaDataType[j] = rxBuffer[i];
    }
    // Parse the GPGGA data 
    if( strncmp( ( const char* )NmeaGpsData.NmeaDataType, ( const char* )NmeaDataTypeGPGGA, 5 ) == 0 )
    {
        // RUI_LOG_PRINTF("%s\r\n",rxBuffer); 
        uint8_t BCC=0,temp=rxBuffer[1];
        char* p_temp1;
        uint8_t f_zz[3]={0};
        p_temp1 = strstr(rxBuffer,"*");
        memcpy(f_zz,p_temp1+1,2);
        BCC = strtol(f_zz,NULL, 16);
        for(uint8_t z=2;z<rxBufferSize;z++)
        {
            if(rxBuffer[z] == '*')break;
            temp ^=  rxBuffer[z];
        } 

        if(BCC != temp)return FAIL;

        // NmeaUtcTime
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 11 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaUtcTime[j] = rxBuffer[i];
        }
        // NmeaLatitude
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 10 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaLatitude[j] = rxBuffer[i];
        }
        // NmeaLatitudePole
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 2 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaLatitudePole[j] = rxBuffer[i];
        }
        // NmeaLongitude
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 11 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaLongitude[j] = rxBuffer[i];
        }
        // NmeaLongitudePole
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 2 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaLongitudePole[j] = rxBuffer[i];
        }
        // NmeaFixQuality
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 2 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaFixQuality[j] = rxBuffer[i];
        }
        // NmeaSatelliteTracked
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 3 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaSatelliteTracked[j] = rxBuffer[i];
        }
        // NmeaHorizontalDilution
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 6 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaHorizontalDilution[j] = rxBuffer[i];
        }
        // NmeaAltitude
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 8 )
            {
                return FAIL;
            }
        }
        memset(&(NmeaGpsData.NmeaAltitude),0,8);
        for( int8_t z=0, j = 0; j < fieldSize; j++,z++, i++ )
        {            
            if(rxBuffer[i] == '.')
            {
                z--;
                continue;
            }
            NmeaGpsData.NmeaAltitude[z] = rxBuffer[i];
        }
        // NmeaAltitudeUnit
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 2 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaAltitudeUnit[j] = rxBuffer[i];
        }
        // NmeaHeightGeoid
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 8 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaHeightGeoid[j] = rxBuffer[i];
        }
        // NmeaHeightGeoidUnit
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 2 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaHeightGeoidUnit[j] = rxBuffer[i];
        }
        GpsFormatGpsData( );
        return SUCCESS;
    }
    else if ( strncmp( ( const char* )NmeaGpsData.NmeaDataType, ( const char* )NmeaDataTypeGPRMC, 5 ) == 0 )
    {    
        // NmeaUtcTime
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 11 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaUtcTime[j] = rxBuffer[i];
        }
        // NmeaDataStatus
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 2 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaDataStatus[j] = rxBuffer[i];
        }
        // NmeaLatitude
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 10 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaLatitude[j] = rxBuffer[i];
        }
        // NmeaLatitudePole
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 2 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaLatitudePole[j] = rxBuffer[i];
        }
        // NmeaLongitude
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 11 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaLongitude[j] = rxBuffer[i];
        }
        // NmeaLongitudePole
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 2 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaLongitudePole[j] = rxBuffer[i];
        }
        // NmeaSpeed
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 8 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaSpeed[j] = rxBuffer[i];
        }
        // NmeaDetectionAngle
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 8 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaDetectionAngle[j] = rxBuffer[i];
        }
        // NmeaDate
        fieldSize = 0;
        while( rxBuffer[i + fieldSize++] != ',' )
        {
            if( fieldSize > 8 )
            {
                return FAIL;
            }
        }
        for( j = 0; j < fieldSize; j++, i++ )
        {
            NmeaGpsData.NmeaDate[j] = rxBuffer[i];
        }

        GpsFormatGpsData( );
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}

void GpsFormatGpsData( void )
{
    if( strncmp( ( const char* )NmeaGpsData.NmeaDataType, ( const char* )NmeaDataTypeGPGGA, 5 ) == 0 )
    {
        HasFix = ( NmeaGpsData.NmeaFixQuality[0] > 0x30 ) ? true : false;
    }
    else if ( strncmp( ( const char* )NmeaGpsData.NmeaDataType, ( const char* )NmeaDataTypeGPRMC, 5 ) == 0 )
    {
        HasFix = ( NmeaGpsData.NmeaDataStatus[0] == 0x41 ) ? true : false;
    }   
    GpsConvertPositionFromStringToNumerical( );
    GpsConvertPositionIntoBinary( );
    GpsConvertAltitudeFromStringToNumerical( );

}

void GpsResetPosition( void )
{
    Altitude = 0xFFFF;
    Latitude = 0;
    Longitude = 0;
    LatitudeBinary = 0;
    LongitudeBinary = 0;
}

static double pi = 3.1415926535897932384626;
//static double x_pi = 3.14159265358979324 * 3000.0 / 180.0;  
static double a = 6378245.0;  
static double ee = 0.00669342162296594323;


static double transformLat(double x, double y) {  
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y  
            + 0.2 * sqrt((x > 0 ? x : (x * -1.0)));  
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;  
    ret += (20.0 * sin(y * pi) + 40.0 * sin(y / 3.0 * pi)) * 2.0 / 3.0;  
    ret += (160.0 * sin(y / 12.0 * pi) + 320 * sin(y * pi / 30.0)) * 2.0 / 3.0;  
    return ret;  
}  

static double transformLon(double x, double y) {  
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1  
            * sqrt((x > 0 ? x : (x * -1.0)));  
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;  
    ret += (20.0 * sin(x * pi) + 40.0 * sin(x / 3.0 * pi)) * 2.0 / 3.0;  
    ret += (150.0 * sin(x / 12.0 * pi) + 300.0 * sin(x / 30.0  
            * pi)) * 2.0 / 3.0;  
    return ret;  
}  


static bool outOfChina(double lat, double lon) {  
    if (lon < 72.004 || lon > 137.8347)  
        return true;  
    if (lat < 0.8293 || lat > 55.8271)  
        return true;  
    return false;  
}  


/** 
 * 84 to GCJ-02 World Geodetic System ==> Mars Geodetic System 
 * 
 * @param lat 
 * @param lon 
 * @return 
 */  
void gps84_To_Gcj02(double lat, double lon, double *lat02, double *lon02) {  
		if (outOfChina(lat, lon)) {  
				*lat02 = lat;
				*lon02 = lon;
				return;
		}  
		double dLat = transformLat(lon - 105.0, lat - 35.0);  
		double dLon = transformLon(lon - 105.0, lat - 35.0);  
		double radLat = lat / 180.0 * pi;  
		double magic = sin(radLat);  
		magic = 1 - ee * magic * magic;  
		double sqrtMagic = sqrt(magic);  
		dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * pi);  
		dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * pi);  
		*lat02 = lat + dLat;  
		*lon02 = lon + dLon;  
		return;  
} 
