//===================================================================
// Includes
//===================================================================
// System Includes
#include <stdint.h>
#include <string.h>
#include <assert.h>
// project Includes
#include "serial.h"
#include "camera.h"
//#include "debug.h"

//===================================================================
// Defines
//===================================================================
// Header Bytes
#define SYNC_BYTE 0x56
#define SERIAL_ID 0x00
#define RESPONSE_BYTE 0x76
// Command IDs
#define COMMAND_STOPCURRENTFRAME 0x00
#define COMMAND_RESUMEFRAME 0x03
#define COMMAND_RESET  0x26
#define COMMAND_GEN_VERSION 0x11
#define COMMAND_READ_DATA 0x30
#define COMMAND_WRITE_DATA 0x31
#define COMMAND_READ_FBUF 0x32
#define COMMAND_GET_FBUF_LEN 0x34
#define COMMAND_FBUF_CTRL 0x36
// Constants
#define CAMERA_BUFFER_SIZE 100
#define CAMERADELAY 10
#define CAMERA_FRAME_CHUNK_SIZE 64
// This was achieved by trial and error.
#define STD_WAIT ((unsigned long) 70000)



//===================================================================
// Constants
//===================================================================
const uint8_t COMMAND_HEADER[] = {SYNC_BYTE, SERIAL_ID};


//===================================================================
// Global variables
//===================================================================
uint8_t camera_buffer[CAMERA_BUFFER_SIZE+1];
uint8_t buffer_length;
uint16_t frame_ptr;


//===================================================================
// Private Function Definitions
//===================================================================
void send_command(uint8_t cmd, uint8_t *command_args, uint8_t args_length)
{
    // The format of every message is :
    // [SYNC BYTE | SERIAL ID BYTE | COMMAND_BYTE | COMMAND_ARG_BYTES]
    serial_write(CAMERA, COMMAND_HEADER, 2);
    serial_write(CAMERA, &cmd, 1);
    serial_write(CAMERA, command_args, args_length);
}

/*!
 * \brief A thin wrapper around the serial layer's read. Reads into the global
 *        buffer.
 * \param [in] numbytes, The maximum number of bytes to attempt to read.
 * \param [in] timeout, The maximum amount of time to wait between receiving
 *             bytes.
 * \return This function returns the amount of bytes that it managed to read
 *         from the serial interface before the timeout.
 */
uint8_t read_response(uint8_t numbytes, uint8_t timeout)
{
    // Ensure that we don't overrun the global recv buffer.
    assert(numbytes <= CAMERA_BUFFER_SIZE);

    buffer_length = serial_read_timeout(CAMERA, camera_buffer,
                                        numbytes, STD_WAIT);
//    print_byte_array(camera_buffer, buffer_length);
    return buffer_length;
}

/*!
 * \brief Verifies that the response to a sent command is correct and well
 *        formed.
 * \warning Only guarantees header is intact!
 * \param [in] cmd, The command byte sent.
 * \return This function returns 1 if the response from the device is well
 *         formed, else it returns 0.
 */
int verify_response(uint8_t cmd)
{
    // A well formed message consists of the following:
    // [RESPONSE_BYTE | SERIAL_ID | cmd | 0x00]
    if ((camera_buffer[0] != RESPONSE_BYTE) ||
        (camera_buffer[1] != SERIAL_ID) ||
        (camera_buffer[2] != cmd) ||
        (camera_buffer[3] != 0x00))
            return 0;

    return 1;
}

/*!
 * \brief An all encompassing function that flushes the read buffer, sends a
 *        command, receives the response and verifies it.
 * \param [in] cmd, The command byte sent.
 * \param [in] command_args, The additional command arguments to send.
 * \param [in] command_arg_length, The length of command_args.
 * \param [in] response_length, The expected length of the response.
 * \param [in] flush_buffer, Decides whether or not the receive buffer should
 *             be flushed.
 * \return This function returns 1 if the response from the device is well
 *         formed, else it returns 0.
 */
int run_command(uint8_t cmd, uint8_t *command_args, uint8_t command_arg_length,
                uint8_t response_length, int flush_buffer)
{
    if (flush_buffer)
        read_response(CAMERA_BUFFER_SIZE, 10);

    send_command(cmd, command_args, command_arg_length);
    uint8_t received_response_length = read_response(response_length, STD_WAIT);
    if (received_response_length != response_length)
        return 0;

    if (!verify_response(cmd))
        return 0;

    return 1;

}

int camera_frame_buff_ctrl(uint8_t command)
{
	uint8_t args[] = {0x1, command};
	return run_command(COMMAND_FBUF_CTRL, args, 2, 5, 1);
}

//===================================================================
// Public Function Definitions
//===================================================================
void cam_init(void)
{
	buffer_length = 0;
	frame_ptr = 0;

    init_serial(CAMERA);
    memset(camera_buffer, 0, CAMERA_BUFFER_SIZE +1);
}

resolution get_image_resolution()
{
    uint8_t command_arguments[] = {0x04, 0x04, 0x01, 0x00, 0x19};
    int command_result = run_command(COMMAND_READ_DATA, command_arguments,
                                     0x05, 0x06, 1);
    if (!command_result)
        return 0xFF;

    return camera_buffer[5];
}

int set_image_resolution(resolution new_resolution)
{
    uint8_t command_arguments[] = {0x05, 0x04, 0x01, 0x00, 0x19,
                                   new_resolution};
    int command_response = run_command(COMMAND_WRITE_DATA, command_arguments,
                                       0x06, 0x05, 1);
    return command_response;
}

int take_picture(void)
{
	frame_ptr = 0;
	return camera_frame_buff_ctrl(COMMAND_STOPCURRENTFRAME);
}

int resume_picture(void)
{
    frame_ptr = 0;
    return camera_frame_buff_ctrl(COMMAND_RESUMEFRAME);
}
uint32_t frame_length(void)
{
	uint8_t args[] = {0x01, 0x00};
	int command_response = run_command(COMMAND_GET_FBUF_LEN, args, 2, 9, 1);

	if(!command_response)
		return 0;

	uint32_t len;
	len = camera_buffer[5];
	len <<= 8;
	len |= camera_buffer[6];
	len <<= 8;
	len |= camera_buffer[7];
	len <<= 8;
	len |= camera_buffer[8];

	return len;
}

int camera_reset(void)
{
  uint8_t args[] = {0x0};

  return run_command(COMMAND_RESET, args, 1, 5, 1);
}



uint8_t *read_picture(uint8_t n)
{
    uint8_t args[] = {0x0C, 0x0, 0x0A, 0, 0, frame_ptr >> 8, frame_ptr & 0xFF,
            0, 0, 0, n, CAMERADELAY >> 8, CAMERADELAY & 0xFF};

    int response = run_command(COMMAND_READ_FBUF, args, sizeof(args), 5, 0);

    if(!response)
        return 0;

    response = read_response(n+5, CAMERADELAY);
    //printf("THIS IS TILTING ME by requesting: %d, and getting %d\n", n+5, response);
    if(!response)
        return 0;

    frame_ptr += n;

    return camera_buffer;
}

read_picture_to_ptr(uint8_t * jpeg_buffer, uint8_t n)
{
    uint8_t args[] = {0x0C, 0x0, 0x0A, 0, 0, frame_ptr >> 8, frame_ptr & 0xFF,
            0, 0, 0, n, CAMERADELAY >> 8, CAMERADELAY & 0xFF};

    int response = run_command(COMMAND_READ_FBUF, args, sizeof(args), 5, 0);

    if(!response)
        return 0;

    int buffer_length = serial_read_timeout(CAMERA, jpeg_buffer + frame_ptr,
            n+5, 1000);

    if(!response)
        return 0;

    frame_ptr += n;

}




