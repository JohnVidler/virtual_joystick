#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "SerialPort.h"

void closeDevice( void );
void createDevice( void );
void writeJoystickValues( void );

struct virtual_stick
{
	int x;
	int y;
	int z;
};

int running = 1;
int fd;
struct uinput_user_dev uidev;
struct input_event     ev;
struct input_event     axis[3];
struct virtual_stick   vStick;

void handleClose( int dummy )
{
	printf( "Stopping...\n" );
	running = 0;
}

void die( int code )
{
	fprintf( stderr, "Err: %x\n", code );
	exit( -1 );
}

int main( int argc, char ** argv )
{
	signal( SIGINT, handleClose );

	vStick.x = 0;
	vStick.y = 0;
	vStick.z = 0;

	createDevice();
	char *portname = "/dev/ttyACM0";
	int serialfd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (serialfd < 0)
	{
		printf( "error %d opening %s: %s", errno, portname, strerror (errno) );
		return -1;
	}

	set_interface_attribs (serialfd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking (serialfd, 0);                // set no blocking

	FILE* serialfp = fdopen( serialfd, "rw" );

	while( running == 1 )
	{
		int x = 0;
		int y = 0;
		int btn = 0;
		if( fscanf( serialfp, "%d, %d, %d\n", &x, &y, &btn ) == 3 )
		{
			if( x != vStick.x || y != vStick.y )
			{
				vStick.x = x;
				vStick.y = y;
				printf( "X: %d\tY: %d\tZ: %d\n", vStick.x, vStick.y, vStick.z );
				writeJoystickValues();
			}
		}
	}

	close( serialfd );

	closeDevice();
	printf( "OK!\n" );

	return 0;
}

void closeDevice( void )
{
	if( ioctl(fd, UI_DEV_DESTROY) < 0 )
		die( 2000 );
}

void createDevice( void )
{
	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if( fd < 0 )
	{
		printf( "Unable to talk to the uinput subsystem!\n" );
		exit( EXIT_FAILURE );
	}

	if( ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0 )
		die( 1 );

	if( ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0 )
		die( 1 );
	if( ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0 )
		die( 1 );
//	if( ioctl(fd, UI_SET_ABSBIT, ABS_Z) < 0 )
//		die( 1 );

	memset( &uidev, 0, sizeof(uidev));
	snprintf( uidev.name, UINPUT_MAX_NAME_SIZE, "virtual-joystick" );
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0xdead;
	uidev.id.product = 0xbeef;
	uidev.id.version = 3;

	// Axis min/max
	uidev.absmin[ABS_X] = -1023;
	uidev.absmax[ABS_X] = 1023;
	uidev.absmin[ABS_Y] = -1023;
	uidev.absmax[ABS_Y] = 1023;
	uidev.absmin[ABS_Z] = -1023;
	uidev.absmax[ABS_Z] = 1023;
	
	// Request that the input be created:
	if( write( fd, &uidev, sizeof(uidev) ) < 0 )
		die( 1000 );
	if( ioctl( fd, UI_DEV_CREATE ) < 0 )
		die( 1001 );
}

void writeJoystickValues()
{
	// Clear the axes structure for new data
	memset( axis, 0, sizeof(ev)*3 );

	axis[0].type = EV_ABS;
	axis[0].code = ABS_X;
	axis[0].value = vStick.x;
	axis[1].type = EV_ABS;
	axis[1].code = ABS_Y;
	axis[1].value = vStick.y;
//	axis[2].type = EV_ABS;
//	axis[2].code = ABS_Z;
//	axis[2].value = vStick.z;

	int ret = write( fd, axis, sizeof(ev) );
	ret += write( fd, axis+1, sizeof(ev) );
//	ret += write( fd, axis+2, sizeof(ev) );

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = 0;
	ev.value = 0;
	if(write(fd, &ev, sizeof(struct input_event)) < 0)
		die( 2 );
}
