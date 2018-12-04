#ifndef OpenKAI_src_Protocol__Mavlink_H_
#define OpenKAI_src_Protocol__Mavlink_H_

#include "../Base/common.h"
#include "../Dependency/mavlink/ardupilotmega/mavlink.h"
#include "../Dependency/mavlink/mavlink_conversions.h"
#include "../IO/_IOBase.h"
#include "../IO/_WebSocket.h"

#define MAV_N_PEER 16
#define MAV_N_CMD_U64 4
#define MAV_N_CMD 256

namespace kai
{

struct Time_Stamps
{
	uint64_t attitude;
	uint64_t battery_status;
	uint64_t global_position_int;
	uint64_t heartbeat;
	uint64_t highres_imu;
	uint64_t local_position_ned;
	uint64_t mission_current;
	uint64_t mount_status;
	uint64_t param_set;
	uint64_t position_target_local_ned;
	uint64_t position_target_global_int;
	uint64_t radio_status;
	uint64_t raw_imu;
	uint64_t rc_channels_override;
	uint64_t rc_channels_raw;
	uint64_t sys_status;

	void init()
	{
		heartbeat = 0;
		sys_status = 0;
		battery_status = 0;
		radio_status = 0;
		local_position_ned = 0;
		global_position_int = 0;
		position_target_local_ned = 0;
		position_target_global_int = 0;
		highres_imu = 0;
		attitude = 0;
		raw_imu = 0;
		rc_channels_override = 0;
		rc_channels_raw = 0;
		mission_current = 0;
		mount_status = 0;
		param_set = 0;
	}
};

// Struct containing information on the MAV we are currently connected to
struct Mavlink_Messages
{
	int sysid;
	int compid;

	mavlink_attitude_t						attitude;
	mavlink_battery_status_t				battery_status;
	mavlink_command_ack_t					command_ack;
	mavlink_global_position_int_t			global_position_int;
	mavlink_heartbeat_t						heartbeat;
	mavlink_highres_imu_t					highres_imu;
	mavlink_local_position_ned_t			local_position_ned;
	mavlink_mission_current_t				mission_current;
	mavlink_mount_status_t					mount_status;
	mavlink_param_set_t						param_set;
	mavlink_position_target_local_ned_t		position_target_local_ned;
	mavlink_position_target_global_int_t	position_target_global_int;
	mavlink_radio_status_t					radio_status;
	mavlink_raw_imu_t						raw_imu;
	mavlink_rc_channels_override_t			rc_channels_override;
	mavlink_rc_channels_raw_t				rc_channels_raw;
	mavlink_sys_status_t					sys_status;

	Time_Stamps time_stamps;

	void init(void)
	{
		attitude.yaw = 0;
		attitude.yawspeed = 0;
		attitude.pitch = 0;
		attitude.pitchspeed = 0;
		attitude.roll = 0;
		attitude.rollspeed = 0;
		global_position_int.alt = 0;
		global_position_int.relative_alt = 0;
		heartbeat.custom_mode = 0;
		mission_current.seq = 0;

		time_stamps.init();
	}
};

struct MAVLINK_PEER
{
	void* m_pPeer;
	uint64_t m_pCmdRoute[MAV_N_CMD_U64]; //index of bit corresponds to Mavlink CMD ID

	void init(void)
	{
		m_pPeer = NULL;
		reset();
	}

	void reset(void)
	{
		for(int i=0; i<MAV_N_CMD_U64; i++)
		{
			m_pCmdRoute[i] = 0xffffffffffffffff;
		}
	}

	bool bCmdRoute(uint32_t iCmd)
	{
		IF_F(iCmd >= MAV_N_CMD);
		return m_pCmdRoute[iCmd / 64] & (1 << (iCmd % 64));
	}

	void setCmdRoute(uint32_t iCmd, bool bON)
	{
		IF_(iCmd >= MAV_N_CMD);

		if(bON)
			m_pCmdRoute[iCmd / 64] |= (1 << (iCmd % 64));
		else
			m_pCmdRoute[iCmd / 64] &= ~(1 << (iCmd % 64));
	}
};

class _Mavlink: public _ThreadBase
{
public:
	_Mavlink();
	~_Mavlink();

	bool init(void* pKiss);
	bool start(void);
	bool draw(void);
	bool console(int& iY);

	//Receive
	void handleMessages();
	bool readMessage(mavlink_message_t &message);

	//Send
	void writeMessage(mavlink_message_t message);

	//Cmd
	void cmdInt(mavlink_command_int_t& D);
	void distanceSensor(mavlink_distance_sensor_t& D);
	void globalVisionPositionEstimate(mavlink_global_vision_position_estimate_t& D);
	void gpsInput(mavlink_gps_input_t& D);
	void globalPositionInt(mavlink_global_position_int_t& D);
	void landingTarget(mavlink_landing_target_t& D);
	void mountConfigure(mavlink_mount_configure_t& D);
	void mountControl(mavlink_mount_control_t& D);
	void mountStatus(mavlink_mount_status_t& D);
	void param_set(mavlink_param_set_t& D);
	void positionTargetLocalNed(mavlink_position_target_local_ned_t& D);
	void positionTargetGlobalInt(mavlink_position_target_global_int_t& D);
	void rcChannelsOverride(mavlink_rc_channels_override_t& D);
	void requestDataStream(uint8_t stream_id, int rate);
	void sendHeartbeat(void);
	void setAttitudeTarget(mavlink_set_attitude_target_t& D);
	void setMode(mavlink_set_mode_t& D);
	void setPositionTargetLocalNED(mavlink_set_position_target_local_ned_t& D);
	void setPositionTargetGlobalINT(mavlink_set_position_target_global_int_t& D);
	void visionPositionDelta(mavlink_vision_position_delta_t& D);
	void visionPositionEstimate(mavlink_vision_position_estimate_t& D);

	//Cmd long
	void clComponentArmDisarm(bool bArm);
	void clDoSetMode(int mode);
	void clDoSetPositionYawThrust(float steer, float thrust);
	void clDoSetServo(int iServo, int PWM);

	//Msg routing
	void setCmdRoute(uint32_t iCmd, bool bON);

public:
	void sendMessage(void);
	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_Mavlink *) This)->update();
		return NULL;
	}

	_IOBase* m_pIO;

	int m_mySystemID;
	int m_myComponentID;
	int m_myType;

	int m_devSystemID;
	int m_devComponentID;
	int m_devType;

	Mavlink_Messages m_msg;
	mavlink_set_position_target_local_ned_t m_initPos;
	mavlink_status_t m_status;

	vector<MAVLINK_PEER> m_vPeer;
};

}

#endif

/*
I believe this is the one
The board itself is 1m.  The outter edge of the red circle is 93cm diameter
each red ring is 7cm wide
the central red circle is 9cm diameter

MAVLink command sequence..
1. OpenKai should send a heartbeat at 1hz.  http://mavlink.org/messages/common#HEARTBEAT
Inside the HEARTBEAT message (and any other message), openkai needs to specify it's sys-id and component-id.
ideally the sys-id should match the vehicle's sysid.
By default the vehicle's sys-id is "1" so you could just hardcode that for now.
Eventually openKai should instead listen for heartbeat messages on it's serial port and should look for the first heartbeat messages with a
type != MAV_TYPE_GCS ("6") and grab that system id and use it.

the component id can be anything except 0.
there's a list of suggested component-ids on this page (search for MAV_COMPONENT) http://mavlink.org/messages/common.  but it doesn't really matter.  Maybe we could use MAV_COMP_ID_PATHPLANNER ("195")

The Heartbeat's "type" should be MAV_TYPE_ONBOARD_CONTROLLER (18)
"base_mode" can be anything.
maybe we will eventually assign values for when OpenKai is providing precision-landing updates vs follow-me updates.. but for now just make it "0" I think
"autopilot" should be "0"
"custom_mode" can be "0"
"system_status" should be MAV_STATE_ACTIVE ("4")

Next, when openkai sees the target is should send LANDING_TARGET messages maybe as quickly as it can.
10hz would be nice.  over 50hz is too fast.
http://mavlink.org/messages/common#LANDING_TARGET
	"time_usec" is just the TX1's system time in sec * 1,000,000.  We don't use this anyway
	"target_num" = "0" (for now anyway)
	"frame" = MAV_FRAME_BODY_NED ("8")
	"angle_x" : the horizontal angle to the target in radians.
		0 = the target is right in the middle of the screen.
		negative means the target is to the left of center,
		positive means it's to the right of center.
		OpenKai will need to know the field-of-view of the camera.
		So for example, I measured the field of view of the Logitech 920c and horizontall it's 70.42degrees).
		So if the center of the target was on the left most edge of the image we would send "angle_x" as -0.61 radians (= -35degrees)
	"angle_y" : the vertical angle to the target in radians.
		0 = middle.  to be honest, I'm not exactly sure if negative is up or down.
	"distance" - we don't use this now.
		It can be estimated even using non-stereo cameras if you know the actual size of the object.
		As you probably know, the apparent diameter of the object halves if it's distance is doubled.
		Anyway, we don't use it anyway for now
	"size_x", "size_y" - the horizontal and vertical size of the object in radians.  Also not used.
 */
