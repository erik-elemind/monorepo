#include "binary_interface.h"

// Forward declarations
static COMMAND_ERRVAL_TYPE readCC(BinaryInterface *bi);
static void onPacketFunction(void *context, const uint8_t *buffer, size_t size);

void bi_init(BinaryInterface *bi, Command *commands, size_t num_commands)
{
    // initialize commands
    bi->commands = commands;
    bi->num_commands = num_commands;

    // initialize packet serial
    init_cobs_packet_serial(&(bi->pSerial), 0);
    // packet handler
    bi->pSerial.on_packet_c = (void *)bi;
    bi->pSerial.on_packet_f = onPacketFunction;

    bw_init(&(bi->bw), bi->bw_buffer, BINARY_WRITE_BUFFER_SIZE, &(bi->pSerial));
}

static void onPacketFunction(void *context, const uint8_t *buffer, size_t size)
{
    if (context == NULL)
    {
        return;
    }
    // recover binary interface pointer
    BinaryInterface *bi = (BinaryInterface *)context;
    
    // callback on packet function
    if( bi->on_packet_f != NULL){
      //bi->on_packet_f();
    }   
    
    // initialize reader.
    br_init(&(bi->br), buffer, size);
    // read command code
    COMMAND_ERRVAL_TYPE cmd_code = readCC(bi);
    binitf_handler_func handler = NULL;
    if (cmd_code.error_ == ERROR_NONE && cmd_code.value_ < bi->num_commands)
    {
        Command command = bi->commands[cmd_code.value_];
        handler = command.binitf_handler_f;
    }
    if (handler != NULL)
    {
        // call handler
        handler(&(bi->br));
    }
}

#if 0
void addHandler(COMMAND_CODE_TYPE cmd_code, std::function<void(Reader *r)> handler)
{
  if (cmd_code < MAX_NUM_COMMANDS)
  {
    handlers[cmd_code] = handler;
  }
}
void removeHandler(COMMAND_CODE_TYPE cmd_code)
{
  if (cmd_code < MAX_NUM_COMMANDS)
  {
    handlers[cmd_code] = NULL;
  }
}
void setMessageHandlerCallback(InputCallback *callback)
{
  handleMessageCallback_ = callback;
}
#endif

bool handleMessagesPolling(BinaryInterface *bi)
{
    // Parse Inputs From User
    update_polling(&(bi->pSerial));
    return true;
}
bool handleMessages(BinaryInterface *bi, uint8_t data)
{
    // Parse Inputs From User
    update(&(bi->pSerial), data);
    return true;
}
BinaryReader *getReader(BinaryInterface *bi)
{
    return &(bi->br);
}
BinaryWriter *getWriter(BinaryInterface *bi)
{
    return &(bi->bw);
}
static COMMAND_ERRVAL_TYPE readCC(BinaryInterface *bi)
{
    return COMMAND_ERRVAL_READ(&(bi->br));
}
void writeCC(BinaryInterface *bi, COMMAND_TYPE cc)
{
    COMMAND_ERRVAL_WRITE(&(bi->bw), cc);
}
