#include <stdint.h>
#include <string.h>
#include "at_cmd.h"
#include "lora_config.h"
#include "rui.h"
#include "bsp.h"

#ifndef SUCCESS
#define SUCCESS     0
#endif

#ifndef FAIL
#define FAIL        1
#endif

#define MAX_ARGV        10
static RUI_LORA_STATUS_T app_lora_status; //record status 
static RUI_RETURN_STATUS rui_return_status;


static uint32_t handle_device_config(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in);
static uint32_t handle_lora_config(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in);
static uint32_t handle_lorap2p_config(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in);
static uint32_t handle_device_status(void);
static uint32_t handle_device_get(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in);
static uint32_t handle_lora_get(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in);
static uint32_t user_set_gps_timeout(uint32_t gpstimeout);
static void user_set_gps_format(GPS_FORMAT fmt);
struct board_config_cmd
{
    char *name;
    board_config_Enum board_enum;
};
struct board_config_cmd cmd_str[]=
{
    "restart",restart,
    "sleep",sleep,
    "boot",boot,
    "status",status,
    "uart",uart,
    "uart_mode",uart_mode,
    "gpio",gpio,
    "adc",adc,
    "i2c",i2c,
    "region",region,
    "channel",channel,     
    "dev_eui",dev_eui,
    "app_eui",app_eui,
    "app_key",app_key,
    "dev_addr",dev_addr,
    "apps_key",apps_key,
    "nwks_key",nwks_key,
    "join_mode",join_mode,
    "work_mode",work_mode,
    "ch_mask",ch_mask,
    "class",class,
    "confirm",confirm,
    "dr",dr,
    "tx_power",tx_power,
    "adr",adr,
    "send_interval",send_interval,
	"multicastenable",multicastenable,
    "multicast_dev_addr",multicast_dev_addr,
    "multicast_apps_key",multicast_apps_key,
    "multicast_nwks_key",multicast_nwks_key,

    "sys_dev_eui",sys_dev_eui,
    "sys_app_eui",sys_app_eui,
    "sys_app_key",sys_app_key,
    "sys_dev_addr",sys_dev_addr,
    "sys_apps_key",sys_apps_key,
    "sys_nwks_key",sys_nwks_key,
    "sys_multicast_dev_addr",sys_multicast_dev_addr,
    "sys_multicast_apps_key",sys_multicast_apps_key,
    "sys_multicast_nwks_key",sys_multicast_nwks_key,
    "default_parameters",default_parameters,
    "dutycycle_enable",dutycycle_enable,
    "send_repeat_cnt",send_repeat_cnt,

    "gps_timeout",gps_timeout,
    "gps_format",gps_format
};
/** Structure for registering CONFIG commands */
struct config_cmd
{
    /** The name of the CONFIG command */
    char *name;
    /** The help text associated with the command */
    //const char *help;
    /** The function that should be invoked for this command. */
    int (*function) (RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in);
};

struct config_cmd config_cmds[] = 
{
    "device",                handle_device_config,
    "lora",                  handle_lora_config,
    "lorap2p",               handle_lorap2p_config,
};

struct config_cmd get_cmds[] = 
{
    "device",                handle_device_get,
    "lora",                  handle_lora_get,
};

static int parse_args(char* str, char* argv[], char **end)
{
    int i = 0;
    char* ch = str;
    while(*ch != '\0') {
        i++;
        /*Check if length exceeds*/
        if (i > MAX_ARGV) {
            return 0;
        }

        argv[i-1] = ch;

        while(*ch != ',' && *ch != '\0' && *ch != '&') {
            // if(*ch == ':' && i == 1) {
            if(*ch == ':' ) {
                break;
            }
            else
                ch++;
        }
        
        if (*ch == '&') {
            *ch = '\0';
            *end = ++ch;
            break;
        } else if (*ch == '\0'){
            *end = NULL;
            break;
        }
        
        *ch = '\0';
        ch++;
        while(*ch == ',') {
            ch++;
        }
    }
    return i;
}

static int read_config_string(RUI_LORA_STATUS_T *config, const char *in)
{
    int i;
    int ret;
    int argc;
    char *argv[MAX_ARGV];
    char *end;

    do
    {
        argc = parse_args(in, argv, &end);

        if((strcmp(argv[1],"gpio") == 0) || (strcmp(argv[1],"adc") == 0))
        {
            if (argc != 3) 
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
        }else if (argc != 2) 
        {
            RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
            return FAIL ;
        }

        for (i = 0; i < sizeof(get_cmds)/sizeof(struct config_cmd); i++) 
        {
            if (strcmp(in, get_cmds[i].name) == 0) 
            {
                ret = get_cmds[i].function(config,argc - 1,&argv[1], NULL);
                if (ret != SUCCESS) 
                {
                    return ret;
                }
                break;
            }
        }  
        if (i == sizeof(get_cmds)/sizeof(struct config_cmd)) 
        {
            RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);
            return FAIL ;
        }  
    }while(end != NULL); 
    return 0;   
}

static int write_config_string(RUI_LORA_STATUS_T *config, char *in)
{
    int i;
    int ret;
    int argc;
    char *argv[MAX_ARGV];
    char *end;
    do 
    {
        argc = parse_args(in, argv, &end);
        if (argc <= 2) 
        {
            if ((strcmp(argv[0], config_cmds[0].name) != 0) && ( (strcmp(argv[0], config_cmds[2].name) != 0)) && ((strcmp(argv[1], "default_parameters") != 0)))
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
        }
        in = end;
        for (i = 0; i < sizeof(config_cmds)/sizeof(struct config_cmd); i++) 
        {
            if (strcmp(argv[0], config_cmds[i].name) == 0) 
            {
                ret = config_cmds[i].function(config, argc - 1, &argv[1], NULL);
                if (ret != SUCCESS) 
                {
                    return ret;
                }
                break;
            }
        }
        if (i == sizeof(config_cmds)/sizeof(struct config_cmd)) 
        {
            RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);
            return FAIL ;
        }          
    }while (end != NULL);   
    return 0; 
}


int write_config(char *in)
{    
    int ret;
    ret = write_config_string(&app_lora_status, in);
    if (ret != SUCCESS) {
        return ret;
    }      
    return ret;
}

int read_config(char *in)
{
    int ret;  
    ret = read_config_string(&app_lora_status, in);
    if (ret != SUCCESS) {
        return ret;
    }
    return ret;
}

static uint32_t handle_device_get(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in)
{
    uint8_t i;
    float x0,y0,z0;
    float f_data;
    RUI_GPIO_ST rui_gpio;
    RUI_UART_BAUDRATE br;
    RUI_I2C_ST I2c_temp;                
    I2c_temp.INSTANCE_ID = 1;
    I2c_temp.PIN_SCL = 18;
    I2c_temp.PIN_SDA = 19;

    for (i = 0; i < sizeof(cmd_str)/sizeof(struct board_config_cmd); i++)
    {
        if (strcmp(argv[0], cmd_str[i].name) == 0)
        {
            break;
        }        
    } 
    if (i == sizeof(cmd_str)/sizeof(struct board_config_cmd)) 
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);
        return FAIL ;            
    }
    
    switch(cmd_str[i].board_enum)
    {
        case gpio:
            if(argc == 2)
            {                
                uint8_t pinVal;
                if(atoi(argv[0]) != 0)
                {
                    RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL ;
                }
                rui_gpio.pin_num = atoi(argv[1]);
                rui_gpio.dir = RUI_GPIO_PIN_DIR_INPUT;
                rui_gpio_init(&rui_gpio);

                rui_return_status = rui_gpio_rw(RUI_IF_READ,&rui_gpio,&pinVal);
                switch(rui_return_status)
                {
                    case RUI_STATUS_OK:RUI_LOG_PRINTF("OK %d\r\n", pinVal);
                        break;
                    case RUI_STATUS_PARAMETER_INVALID:
                        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        return FAIL ;
                    default:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);return FAIL ;
                }                
                // rui_gpio_uninit(&rui_gpio);
            }else 
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
            break;         
        case adc:
            if(argc == 2)
            {
                uint16_t adc_value;
                rui_gpio.pin_num = atoi(argv[1]);
                rui_gpio.dir = RUI_GPIO_PIN_DIR_INPUT;

                rui_return_status = rui_adc_init(&rui_gpio);
                switch(rui_return_status)
                {
                    case RUI_STATUS_OK:break;
                    case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        rui_adc_uninit(&rui_gpio);
                        return FAIL;
                    default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        rui_adc_uninit(&rui_gpio);
                        return FAIL;
                }

                rui_return_status = rui_adc_get(&rui_gpio,&adc_value);
                switch(rui_return_status)
                {
                    case RUI_STATUS_OK:RUI_LOG_PRINTF("OK %dmV\r\n",adc_value);
                        rui_adc_uninit(&rui_gpio);
                        break;
                    case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        rui_adc_uninit(&rui_gpio);
                        return FAIL;
                    default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        rui_adc_uninit(&rui_gpio);
                        return FAIL;
                }                 
            }
            else 
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
            break;
        case status:handle_device_status();
            break;
        default :RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);return FAIL ;
            break;
    }
    return SUCCESS;

}
uint32_t verify_isdigit(uint8_t* data,uint32_t len)
{
    for(uint32_t i=0;i<len;i++)
    {
        if(isdigit(data[i])==0)
        {
            return FAIL ;
        } 
    }
    return SUCCESS;
}
static uint32_t handle_device_config(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in)
{
    uint8_t i;
    float x0,y0,z0;
    float f_data;
    RUI_GPIO_ST rui_gpio;
    RUI_UART_BAUDRATE br;
    RUI_I2C_ST I2c_temp;                
    I2c_temp.INSTANCE_ID = 1;
    I2c_temp.PIN_SCL = 18;
    I2c_temp.PIN_SDA = 19;
    for (i = 0; i < sizeof(cmd_str)/sizeof(struct board_config_cmd); i++)
    {
        if (strcmp(argv[0], cmd_str[i].name) == 0)
        {
            break;
        }        
    } 
    if (i == sizeof(cmd_str)/sizeof(struct board_config_cmd)) 
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);
        return FAIL ;            
    }
    
    switch(cmd_str[i].board_enum)
    {
        case restart:
            // RUI_LOG_PRINTF("OK \r\n");
            rui_delay_ms(10);
            rui_device_reset();
            break;
        case sleep:
            if(argc != 2)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            } 
            if(atoi(argv[1])>1) 
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL; 
            }         
            rui_return_status = rui_device_sleep(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:
                    return SUCCESS;
                case RUI_LORA_STATUS_BUSY:
                    RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_LORA_BUSY);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);return FAIL;
            }            
            break; 
        case boot:
            if(argc>1)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			}
            RUI_LOG_PRINTF("OK \r\n");
            rui_device_boot();  
            break;  
        case uart:
            if(verify_isdigit(argv[2],strlen(argv[2]))==FAIL)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            } 
            if(argc != 3)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }else
            {
                switch(atoi(argv[2]))
                {
                    case 1200:br = BAUDRATE_1200;
                        break;
                    case 2400:br = BAUDRATE_2400;
                        break;
                    case 4800:br = BAUDRATE_4800;
                        break;
                    case 9600:br = BAUDRATE_9600;
                        break;
                    case 19200:br = BAUDRATE_19200;
                        break;
                    case 38400:br = BAUDRATE_38400;
                        break;
                    case 57600:br = BAUDRATE_57600;
                        break;
                    case 115200:br = BAUDRATE_115200;
                        break;
                    default:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);return FAIL;
                    break;
                }
                rui_return_status = rui_uart_init(atoi(argv[1]),br);
                switch(rui_return_status)
                {
                    case RUI_STATUS_OK:
                        RUI_LOG_PRINTF("OK \r\n");
                        return SUCCESS;
                    case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        return FAIL;
                    case RUI_STATUS_RW_FLASH_ERROR:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_RW_FLASH_ERROR);
                        return FAIL;
                }                
            }
            break; 
        case uart_mode:
            if (g_lora_config.work_mode != LORAWAN)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_LORA_SERVICE_UNKNOWN);
                return FAIL ;
            }
            if((verify_isdigit(argv[1],strlen(argv[1]))==FAIL) || (verify_isdigit(argv[2],strlen(argv[2]))==FAIL))
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            } 
            if(argc != 3)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }else 
            {
                if(atoi(argv[2]) != 1)
                {
                    RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL ;   
                }
                rui_lora_get_status(false,&app_lora_status);//The query gets the current device status 
                switch(atoi(argv[1]))
                {
                    case RUI_UART1:
                        if(app_lora_status.uart1_mode == RUI_UART_NORMAL)rui_return_status = rui_uart_mode_config(RUI_UART1,atoi(argv[2]));
                        else rui_return_status = RUI_STATUS_PARAMETER_INVALID;
                        break;
                    case RUI_UART2:
                        if(app_lora_status.uart2_mode == RUI_UART_NORMAL)rui_return_status = rui_uart_mode_config(RUI_UART2,atoi(argv[2]));
                        else rui_return_status = RUI_STATUS_PARAMETER_INVALID;
                        break;
                    case RUI_UART3:
                        if(app_lora_status.uart3_mode == RUI_UART_NORMAL)rui_return_status = rui_uart_mode_config(RUI_UART3,atoi(argv[2]));
                        else rui_return_status = RUI_STATUS_PARAMETER_INVALID;
                        break;
                    default:rui_return_status = RUI_STATUS_PARAMETER_INVALID;
                }

                switch(rui_return_status)
                {
                    case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");                        
                        break;
                    case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        return FAIL;
                }
            }

            break;
        case gpio:
            if((verify_isdigit(argv[1],strlen(argv[1]))==FAIL) || (verify_isdigit(argv[2],strlen(argv[2]))==FAIL))
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }  
            if(argc == 3)
            {
                uint8_t pinVal;
                pinVal = atoi(argv[2]);
                rui_gpio.pin_num = atoi(argv[1]);
                rui_gpio.dir = RUI_GPIO_PIN_DIR_OUTPUT;
                rui_gpio_init(&rui_gpio);
                rui_return_status = rui_gpio_rw(RUI_IF_WRITE,&rui_gpio,&pinVal);
                switch(rui_return_status)
                {
                    case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                        return SUCCESS;
                    case RUI_STATUS_PARAMETER_INVALID:
                        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        rui_gpio_uninit(&rui_gpio);
                        return FAIL ;
                    default:
                        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                        rui_gpio_uninit(&rui_gpio);
                        return FAIL ;
                }
            }
            else 
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
            break;         

        case gps_format:
            if(argc != 2)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            } 
            if(atoi(argv[1])>1) 
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL; 
            }         
            user_set_gps_format(atoi(argv[1]));
            break;

        case gps_timeout:
            if(argc != 2)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }       
            user_set_gps_timeout(atoi(argv[1]));
            break;
        default :RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);return FAIL ;
            break;
    }
    return SUCCESS;

}

static int verify_config_data(uint8_t argc,char* buffer,char len,char* lora_id)
{
    char hex_num[3] = {0};
    if (argc != 2) 
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL ;
    }             
    if (strlen(buffer) != 2*len) {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL ;
    }
    for (int i = 0; i < 2*len; i++) {
        if (!isxdigit(buffer[i])) {
            RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
            return FAIL ;    
        }
    }
    for (int i = 0; i < len; i++) {
        memcpy(hex_num, &buffer[i*2], 2);
        lora_id[i] = strtoul(hex_num, NULL, 16);
    }
   return SUCCESS;
}

static uint32_t handle_lora_get(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in)
{
    uint8_t i;
    char lora_id[32];
    RUI_LORA_STATUS_T* st;

    for (i = 0; i < sizeof(cmd_str)/sizeof(struct board_config_cmd); i++)
    {
        if (strcmp(argv[0], cmd_str[i].name) == 0)
        {
            break;
        }        
    }
    if (i == sizeof(cmd_str)/sizeof(struct board_config_cmd)) 
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);
        return FAIL ;
    }     

   switch(cmd_str[i].board_enum)
    {
        case channel:rui_return_status = rui_get_channel_list();
            break;
        case status:
            rui_return_status = rui_lora_get_status(true,st);
            break;
        default :RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);
            return FAIL ;
            break;
    }  
    return SUCCESS;  
}


static uint32_t handle_lora_config(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in)
{
    uint8_t i;
    char lora_id[32];
    RUI_LORA_STATUS_T* st;

    for (i = 0; i < sizeof(cmd_str)/sizeof(struct board_config_cmd); i++)
    {
        if (strcmp(argv[0], cmd_str[i].name) == 0)
        {
            break;
        }        
    }
    if (i == sizeof(cmd_str)/sizeof(struct board_config_cmd)) 
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);
        return FAIL ;
    }     
// UartPrint("isdigit:%d, argv[1]:%d\r\n",isdigit(argv[1][0]),atoi(argv[1]));
   switch(cmd_str[i].board_enum)
    {
        case region:
            rui_lora_get_status(false,&app_lora_status);
            if ( 0==strcmp(argv[1], app_lora_status.region)) 
            { 
                // RUI_LOG_PRINTF("Selected LoRaWAN 1.0.2 Region: %s\r\n",app_lora_status.region);
                RUI_LOG_PRINTF("OK \r\n");
                return SUCCESS;
            } 
            else 
            {	
                if (rw_String2Region(argv[1]) == 100) 
                {
                    RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_LORA_REGION_NOT_SUPPORTED);
                    return FAIL;
                }
                else
                {
                    rui_return_status = rui_lora_set_region(rw_String2Region(argv[1]));
                    switch(rui_return_status)
                    {
                        case RUI_STATUS_OK:	RUI_LOG_PRINTF("OK \r\n");
                            return SUCCESS;
                        case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                            return FAIL;
                        default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                            return FAIL;
                    } 
                }                
            }
            break;
        case channel:rui_return_status = rui_get_channel_list();
            break;
        case status:
            rui_return_status = rui_lora_get_status(true,st);
            break;
        case dev_eui:            
            if(verify_config_data(argc,argv[1],8,lora_id) != SUCCESS)return FAIL ;        
            rui_return_status = rui_lora_set_dev_eui(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            } 
            break;
        case app_eui:
            if(verify_config_data(argc,argv[1],8,lora_id) != SUCCESS)return FAIL ; 
            rui_return_status = rui_lora_set_app_eui(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            } 
            break;
        case app_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS) return FAIL ;  
            rui_return_status = rui_lora_set_app_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }         
            break;
        case dev_addr:            
            if(verify_config_data(argc,argv[1],4,lora_id) != SUCCESS)return FAIL ;  
            rui_return_status = rui_lora_set_dev_addr(lora_id); 
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }    
            break;
        case apps_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_apps_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }    
            break;
        case nwks_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_nwks_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }             
            break;
        case multicast_dev_addr:
            if(verify_config_data(argc,argv[1],4,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_multicast_dev_addr(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break;
        case multicast_apps_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_multicast_apps_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break;
        case multicast_nwks_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_multicast_nwks_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break;
        case multicastenable:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			} 
            if(atoi(argv[1])>1)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL;
            }
            rui_return_status = rui_lora_set_multicastenable(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");                          
                    break;  
                default:RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL; 
            }               
            break;                
        case join_mode:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			} 
            rui_return_status = rui_lora_set_join_mode(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:
                    switch(atoi(argv[1]))
                    {
                        case RUI_OTAA:RUI_LOG_PRINTF("OK \r\n");
                            break;
                        case RUI_ABP:RUI_LOG_PRINTF("OK \r\n");
                            break;
                    }
                    break;  
                default:RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL; 
            }               
            break;
        case work_mode:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			} 
            // if(g_lora_config.work_mode == atoi(argv[1]))
            // {
            //     RUI_LOG_PRINTF("OK \r\n");
            //     return SUCCESS;
            // }
            if(atoi(argv[1])>1)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL;
            }
            rui_return_status = rui_lora_set_work_mode(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:
                    switch (atoi(argv[1]))
                    {
                        case RUI_LORAWAN:
                            rui_device_reset();
                            break;	
                        case RUI_P2P:
                            rui_device_reset();
                            break;	
                        default:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                            return FAIL;
                    }
                    return SUCCESS;
                case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL;
            }        
            break;
        case ch_mask:
            if((verify_isdigit(argv[1],strlen(argv[1]))==FAIL) || (verify_isdigit(argv[2],strlen(argv[2]))==FAIL))
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
            rui_return_status = rui_lora_set_channel_mask(atoi(argv[1]),atoi(argv[2]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL;
            } 
            break;
        case class:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			}
            rui_return_status = rui_lora_set_class(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:
                    switch(atoi(argv[1]))
                    {
                        case RUI_CLASS_A:RUI_LOG_PRINTF("OK \r\n");
                            break;
                        case RUI_CLASS_B:RUI_LOG_PRINTF("OK \r\n");
                            break;
                        case RUI_CLASS_C:RUI_LOG_PRINTF("OK \r\n");
                            break;
                    }
                    return SUCCESS;
                case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL;
            }                     
            break;
        case confirm:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			} 
        	if(atoi(argv[1]) > 2)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL;
            }
            rui_return_status = rui_lora_set_confirm(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:
                    switch(atoi(argv[1]))
                    {
                        case 0:
                        case 1:
                        case 2:
                            RUI_LOG_PRINTF("OK \r\n");
                            break;
                        default: RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                            return FAIL;
                    }
                    break; 
                case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }      
            break;   
        case dr:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			}
            rui_return_status = rui_lora_set_dr(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n",atoi(argv[1]));
                    return SUCCESS;
                case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL;
            }        
            break;        
        case adr:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			}
            if(atoi(argv[1])>1)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL;
            }
            rui_return_status = rui_lora_set_adr(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:
                    switch(atoi(argv[1]))
                    {
                        case 0:RUI_LOG_PRINTF("OK \r\n");
                            break;
                        case 1:RUI_LOG_PRINTF("OK \r\n");
                            break;
                        default:
                            break;
                    }
                    return SUCCESS;
                case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL;
            }         
            break; 
        case tx_power:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			}
            rui_return_status = rui_lora_set_tx_power(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL;
            } 
            break; 
        case send_interval:
            if((verify_isdigit(argv[1],strlen(argv[1]))==FAIL) || (verify_isdigit(argv[2],strlen(argv[2]))==FAIL))
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            } 
            if (argc != 3)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            } 
            if(atoi(argv[1]) > 2)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL ;
            }
           
            rui_return_status = rui_lora_set_send_interval(atoi(argv[1]),atoi(argv[2]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:
                    switch(atoi(argv[1]))
                    {
                        case 0:RUI_LOG_PRINTF("OK \r\n");
                            break;
                        case 1:RUI_LOG_PRINTF("OK \r\n");
                            break;
                        case 2:RUI_LOG_PRINTF("OK \r\n");
                            break;
                    }
                    return SUCCESS;
                case RUI_STATUS_PARAMETER_INVALID:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",rui_return_status);
                    return FAIL;
            } 
            break;
        case sys_dev_eui:            
            if(verify_config_data(argc,argv[1],8,lora_id) != SUCCESS)return FAIL ;        
            rui_return_status = rui_lora_set_sys_dev_eui(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            } 
            break;
        case sys_app_eui:
            if(verify_config_data(argc,argv[1],8,lora_id) != SUCCESS)return FAIL ; 
            rui_return_status = rui_lora_set_sys_app_eui(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            } 
            break;
        case sys_app_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS) return FAIL ;  
            rui_return_status = rui_lora_set_sys_app_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }         
            break;
        case sys_dev_addr:            
            if(verify_config_data(argc,argv[1],4,lora_id) != SUCCESS)return FAIL ;  
            rui_return_status = rui_lora_set_sys_dev_addr(lora_id); 
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }    
            break;
        case sys_apps_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_sys_apps_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }    
            break;
        case sys_nwks_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_sys_nwks_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }             
            break;
        case sys_multicast_dev_addr:
            if(verify_config_data(argc,argv[1],4,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_sys_multicast_dev_addr(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break;
        case sys_multicast_apps_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_sys_multicast_apps_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break;
        case sys_multicast_nwks_key:
            if(verify_config_data(argc,argv[1],16,lora_id) != SUCCESS)return FAIL ;
            rui_return_status = rui_lora_set_sys_multicast_nwks_key(lora_id);
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break; 
        case default_parameters:
            // UartPrint("argc:%d\r\n",argc);
            if(argc>1)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			}
            rui_return_status = rui_lora_set_sys_default_parameters();
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break; 
        case dutycycle_enable:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			}
            if(atoi(argv[1])>1)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL;
            }
            rui_return_status = rui_lora_set_dutycycle_enable(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break; 
        case send_repeat_cnt:
            if(verify_isdigit(argv[1],strlen(argv[1]))==FAIL)
			{
				RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
				return FAIL ;
			}
            if(atoi(argv[1])>8)
            {
                RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL;
            }
            rui_return_status = rui_lora_set_send_repeat_cnt(atoi(argv[1]));
            switch(rui_return_status)
            {
                case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                    return SUCCESS;
                default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                    return FAIL;
            }
            break;          
        default :RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_UNSUPPORT);
            return FAIL ;
            break;
    }  
    return SUCCESS;  
}

static uint32_t  handle_lorap2p_config(RUI_LORA_STATUS_T *config, int argc, char *argv[], char *in)
{ 
    uint32_t Frequency;
    uint8_t  Spreadfact;
    uint8_t  Bandwidth; 
    uint8_t  Codingrate; 
    uint16_t  Preamlen; 
    uint8_t  Powerdbm;
    if(g_lora_config.work_mode==LORAWAN)
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_LORA_STATUS_SERVICE_UNKNOWN);
        return FAIL;
    }
    if(strcmp(argv[0],"transfer_mode")==0)
    {
        // RUI_LOG_PRINTF("argc: %d\r\n",argc);
        if((argc > 2)||(argc < 1))
        {
            RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
            return FAIL ;
        }

        rui_return_status=rui_lorap2p_set_work_mode(atoi(argv[1]));
        switch(rui_return_status)
        {
            case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
                return SUCCESS;
            case RUI_LORA_STATUS_SERVICE_UNKNOWN:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_LORA_STATUS_SERVICE_UNKNOWN);
                return FAIL;
            default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
                return FAIL;
        }      
    }

    if(argc > 6)
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL ;
    }

    for(uint8_t i=0; i<6; i++)
    {
        if(verify_isdigit(argv[i],strlen(argv[i]))==FAIL)
        {
            RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
            return FAIL ;
        }
    }    
    if((atoi(argv[0])>>(sizeof(uint32_t))*8)!=0)
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL;
    }else  Frequency=atoi(argv[0]);
    // RUI_LOG_PRINTF("Frequency=%d\r\n",Frequency);

    if((atoi(argv[1])>12)||(atoi(argv[1])<7))
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL ;
    } else Spreadfact = atoi(argv[1]);

    if((atoi(argv[2])>2)||(atoi(argv[2])<0))
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL ;
    } else Bandwidth = atoi(argv[2]);

    if((atoi(argv[3])>4)||(atoi(argv[3])<1))
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL ;
    } else Codingrate = atoi(argv[3]);

    if((atoi(argv[4])>65535)||(atoi(argv[4])<2))
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL ;
    } else Preamlen = atoi(argv[4]);

    if((atoi(argv[5])>20)||(atoi(argv[5])<0))
    {
        RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
        return FAIL ;
    } else Powerdbm = atoi(argv[5]);

    rui_return_status=rui_lorap2p_config(Frequency,Spreadfact,Bandwidth,Codingrate,Preamlen,Powerdbm);
    switch(rui_return_status)
    {
        case RUI_STATUS_OK:RUI_LOG_PRINTF("OK \r\n");
            return SUCCESS;
        case RUI_LORA_STATUS_SERVICE_UNKNOWN:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_LORA_STATUS_SERVICE_UNKNOWN);
            return FAIL;
        case RUI_LORA_STATUS_BUSY:RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_LORA_STATUS_BUSY);
            return FAIL;
        default: RUI_LOG_PRINTF("ERROR: %d\r\n",RUI_AT_PARAMETER_INVALID);
            return FAIL;
    }

}


extern bsp_sensor_data_t bsp_sensor;
extern TimerEvent_t Gps_Cnt_Timer;  //search satellite timer
extern user_store_data_t user_store_data;
extern bool HasFix;
extern bool sample_flag ;
static uint32_t handle_device_status(void)
{
    double latitude_tmp, longitude_tmp = 0;
    double gps_lat, gps_lon;
	int32_t gps_alt;
    RUI_LOG_PRINTF("OK.\r\n*************************************************\r\n===============Device Status List================\r\n"); 

    RUI_LOG_PRINTF("Board Core:  RAK811\r\n");
    RUI_LOG_PRINTF("MCU:  STM32L151CBU6A\r\n");   
    RUI_LOG_PRINTF("LoRa chip:  SX1276\r\n"); 
    RUI_LOG_PRINTF("\r\n"); 

    if(sample_flag)  
    {
        /*If sampled sensor data, print send sensor data here*/
        RUI_LOG_PRINTF("Battery Voltage:%d.%d V \r\n",(uint32_t)(bsp_sensor.voltage), (uint32_t)((bsp_sensor.voltage)*1000-((int32_t)(bsp_sensor.voltage)) * 1000));
        RUI_LOG_PRINTF("\r\n"); 

        RUI_LOG_PRINTF("gps_timeout: %ds\r\n",user_store_data.gps_timeout_cnt);
        switch(user_store_data.gps_format)
        {
            case POINT_BIT4:RUI_LOG_PRINTF("gps_format:standard LPP format\r\n");
                break;
            case POINT_BIT6:RUI_LOG_PRINTF("gps_format:GPS take six decimal places\r\n");
                break;
            default:break;
        }
        
        RUI_LOG_PRINTF("GPS data:\r\n");
        if(HasFix) 
        {            
            RUI_LOG_PRINTF("  latitude: %d.%d, longitude: %d.%d, altitude: %d.%dm \r\n",
                                (int32_t)bsp_sensor.latitude,abs((int32_t)(bsp_sensor.latitude*1000000-((int32_t)bsp_sensor.latitude) * 1000000)),
                                (int32_t)bsp_sensor.longitude,abs((int32_t)(bsp_sensor.longitude*1000000-((int32_t)bsp_sensor.longitude) * 1000000)),    
                                bsp_sensor.altitude/10,abs(bsp_sensor.altitude%10));
        }else RUI_LOG_PRINTF("  No signal with Satellite.\r\n");                    
        RUI_LOG_PRINTF("\r\n");                     

        RUI_LOG_PRINTF("LIS3DH sensor data:\r\n");
        RUI_LOG_PRINTF("  ACC_X: %dmg, ACC_Y: %dmg, ACC_Z: %dmg\r\n",(int32_t)bsp_sensor.triaxial_x , (int32_t)bsp_sensor.triaxial_y , (int32_t)bsp_sensor.triaxial_z );
        RUI_LOG_PRINTF("\r\n"); 

        RUI_LOG_PRINTF("BME680 sensor data:\r\n");
        RUI_LOG_PRINTF("  Humidity:%d.%d %%RH\r\n",(int32_t)(bsp_sensor.humidity/1000),(int32_t)(bsp_sensor.humidity%1000));		
        RUI_LOG_PRINTF("  Temperature:%d.%d degree\r\n",(int32_t)(bsp_sensor.temperature/100),(int32_t)(bsp_sensor.temperature%100));	
        RUI_LOG_PRINTF("  Pressure:%d.%d hPa\r\n",(int32_t)(bsp_sensor.pressure/100),(int32_t)(bsp_sensor.pressure%100));	
        RUI_LOG_PRINTF("  Gas_resistance: %d Ohms \r\n", bsp_sensor.resis);	
    }else
    {
        /*If not sampled sensor data, print current sensor data here */
        BoardBatteryMeasureVolage(&bsp_sensor.voltage);
        bsp_sensor.voltage=bsp_sensor.voltage/1000.0;   //convert mV to V
        RUI_LOG_PRINTF("Battery Voltage:%d.%d V \r\n",(uint32_t)(bsp_sensor.voltage), (uint32_t)((bsp_sensor.voltage)*1000-((int32_t)(bsp_sensor.voltage)) * 1000));
        RUI_LOG_PRINTF("\r\n"); 

        RUI_LOG_PRINTF("gps_timeout: %ds\r\n",user_store_data.gps_timeout_cnt);
        switch(user_store_data.gps_format)
        {
            case POINT_BIT4:RUI_LOG_PRINTF("gps_format:standard LPP format\r\n");
                break;
            case POINT_BIT6:RUI_LOG_PRINTF("gps_format:GPS take six decimal places\r\n");
                break;
            default:break;
        }
        RUI_LOG_PRINTF("GPS data:\r\n");
        if(HasFix) 
        {            
            GpsGetLatestGpsPositionDouble(&latitude_tmp, &longitude_tmp);
			gps84_To_Gcj02(latitude_tmp, longitude_tmp, &gps_lat, &gps_lon);
			GpsGetLatestGpsAltitude(&gps_alt);
            RUI_LOG_PRINTF("  latitude: %d.%d, longitude: %d.%d , altitude: %d.%dm \r\n",
						(int32_t)gps_lat,abs((int32_t)(gps_lat*1000000-((int32_t)gps_lat) * 1000000)),
						(int32_t)gps_lon,abs((int32_t)(gps_lon*1000000-((int32_t)gps_lon) * 1000000)),    
						gps_alt/10,abs(gps_alt%10));
        }else RUI_LOG_PRINTF("  No signal with Satellite.\r\n");
        RUI_LOG_PRINTF("\r\n");        
        
        lis3dh_get_data(&bsp_sensor.triaxial_x,&bsp_sensor.triaxial_y,&bsp_sensor.triaxial_z);
        RUI_LOG_PRINTF("\r\n"); 
        
        BME680_get_data(&bsp_sensor.humidity,&bsp_sensor.temperature,&bsp_sensor.pressure,&bsp_sensor.resis); 
    }
    
    RUI_LOG_PRINTF("===================List End======================\r\n"); 
    RUI_LOG_PRINTF("*************************************************\r\n");       
}

static uint32_t user_set_gps_timeout(uint32_t gpstimeout)
{
    if(gpstimeout==0)gpstimeout=1;
    if(user_store_data.gps_timeout_cnt != gpstimeout)
    {
        user_store_data.gps_timeout_cnt = gpstimeout;
        if(rui_flash_write(RUI_FLASH_USER,&user_store_data,sizeof(user_store_data)) == RUI_STATUS_PARAMETER_INVALID )
        {
            RUI_LOG_PRINTF("the length over size.\r\n");
            return;
        }
    }
    rui_timer_stop(&Gps_Cnt_Timer); 
    rui_timer_setvalue( &Gps_Cnt_Timer, user_store_data.gps_timeout_cnt * 1000 );
    rui_timer_start(&Gps_Cnt_Timer); //restart search satellite timer
    
    RUI_LOG_PRINTF("OK\r\n");
}

static void user_set_gps_format(GPS_FORMAT fmt)
{
    if(fmt > 1)
    {
        RUI_LOG_PRINTF("Parameter is invalid.\r\n");
        return;
    }else
    {
        RUI_LOG_PRINTF("OK\r\n");
    }
    
    if(fmt != user_store_data.gps_format)
    {
        user_store_data.gps_format = fmt;
        if(rui_flash_write(RUI_FLASH_USER,&user_store_data,sizeof(user_store_data)) == RUI_STATUS_PARAMETER_INVALID )
        {
            RUI_LOG_PRINTF("the length over size.\r\n");
        }
    }

}

