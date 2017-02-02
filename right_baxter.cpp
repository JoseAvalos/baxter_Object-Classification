#include <ros/ros.h>
#include <ros/package.h>
#include <rbdl/rbdl.h>
#include <sensor_msgs/JointState.h>
#include <geometry_msgs/TransformStamped.h>
#include <robot-model/robot-model.hpp>
#include <tf2_msgs/TFMessage.h>
#include <osik-control/math-tools.hpp>
#include <osik-control/kine-task.hpp>
#include <osik-control/kine-task-pose.hpp>
#include <osik-control/kine-solver-WQP.hpp>
#include <baxter_obj_classification/robotSpecifics.h>
#include <baxter_obj_classification/tools.hpp>
#include <baxter_core_msgs/JointCommand.h>
#include <baxter_core_msgs/EndpointState.h>
#include <math.h>
#include <stdlib.h>
#include <cmath>
#include <Eigen/Dense>
#include <visualization_msgs/Marker.h>
#include <baxter_obj_classification/markers.hpp>
#include <tf/transform_datatypes.h>
#include <fstream>
#include <unistd.h>
#include <tf/transform_listener.h>
#include <Eigen/Geometry>


using namespace osik;


class EndPoint
{
    
    public:
    EndPoint()
    : msg_(new baxter_core_msgs::EndpointState)
    {
        
        std::cout << "EndPoint" <<msg_->pose.position.x << std::endl;
        
    }
    
    void readEndPoint(const baxter_core_msgs::EndpointState::ConstPtr& msg)
    {
        
        msg_ = msg;
        
    }
    
    baxter_core_msgs::EndpointState::ConstPtr getEndPoint()
    {
        
        return msg_;
        
    }
    
    private:
    baxter_core_msgs::EndpointState::ConstPtr msg_;
    
}
;


class DataVector
{
    
    public:
    DataVector()
    : msg_(new geometry_msgs::Vector3)
    {
        
        std::cout << "Mouse: " <<msg_->x << std::endl;
        
    }
    
    void readDataVector(const geometry_msgs::Vector3::ConstPtr& msg)
    {
        
        msg_ = msg;
        
    }
    
    geometry_msgs::Vector3::ConstPtr getData()
    {
        
        return msg_;
        
    }
    
    private:
    geometry_msgs::Vector3::ConstPtr msg_;
    
}
;



int main(int argc, char **argv)
{
      
    //############################################
    //INFORMATION OF THE ROBOT - BAXTER
    //############################################
    std::string baxter_description = ros::package::getPath("baxter_description");
    std::string model_name = baxter_description + "/urdf/baxter.urdf";
    RobotModel* robot = new RobotModel();
    bool has_floating_base = false;
    if (!robot->loadURDF(model_name, has_floating_base)){
        
        return -1;
        
    }
    
    else{
        
        std::cout << "Robot " << model_name << " loaded." << std::endl;
        
    }
    
    unsigned int ndof_full = robot->ndof();
    std::cout << "Reading initial sensor values ..." << ndof_full<< std::endl;
    // Get the joint names and joint limits
    std::vector<std::string> jnames;
    std::vector<double> qmin, qmax, dqmax;
    jnames = robot->jointNames();
    qmin = robot->jointMinAngularLimits();
    qmax = robot->jointMaxAngularLimits();
    dqmax = robot->jointVelocityLimits();
    //############################################
    //NODE
    //############################################
    ros::init(argc, argv, "right_sendbaxter");
    ros::NodeHandle nh;
    Eigen::VectorXd P_right_wrist;
    //Eigen::VectorXd P_left_elbow;
    Eigen::VectorXd P_right_elbow;
    //Eigen::VectorXd P_left_wrist;
    
    for(int i=0;i<jnames.size();i++){
        
        std::cout<<jnames[i]<<std::endl;
        
    }
    
    //############################################
    //Suscriber
    //############################################
    
    ros::Rate iter_rate(100000); // Hz
    unsigned int niter=0, max_iter = 1e3;
    unsigned int ndof_sensed = 222;
    
    DataVector right_position_baxter;
   //DataVector left_position_baxter;


    ros::Subscriber sub_right = nh.subscribe("right_position_baxter", 1000 , &DataVector::readDataVector, &right_position_baxter);
    //ros::Subscriber sub_left = nh.subscribe("left_position_baxter", 1000 , &DataVector::readDataVector, &left_position_baxter);
    
    ros::spinOnce();
    iter_rate.sleep();
    Eigen::VectorXd qsensed_right = Eigen::VectorXd::Zero(ndof_full);
    //Eigen::VectorXd qsensed_left = Eigen::VectorXd::Zero(ndof_full);
    
    Eigen::Vector3d w(0,0,0);
    Eigen::Vector3d v(0,0,0);
    Eigen::Vector3d qwe(0,0,0);
    
    Eigen::MatrixXd ball_position(3,100);
    Eigen::MatrixXd frame_pose(6,100);
    int link_number=
    {
        
        1
        
    }
    ;
    //Eigen::VectorXd qposition=Eigen::MatrixXd::Constant(15, 1, 0.0);

    //############################################
    //INICIO DEL PROCESO DE ENVIO
    //############################################
    ros::Publisher pub = nh.advertise<sensor_msgs::JointState>("joint_states", 5000);
    //ros::Publisher pub_left = nh.advertise<baxter_core_msgs::JointCommand>("robot/limb/left/joint_command", 5000);
    ros::Publisher pub_right = nh.advertise<baxter_core_msgs::JointCommand>("robot/limb/right/joint_command", 5000);
    
    sensor_msgs::JointState jcmd;
    //baxter_core_msgs::JointCommand jcmd_left;
    baxter_core_msgs::JointCommand jcmd_right;
    
    //jcmd_left.mode=1;
    //jcmd_left.names.resize(7);
    //jcmd_left.command.resize(7);
    
    jcmd_right.mode=1;
    jcmd_right.names.resize(7);
    jcmd_right.command.resize(7);
    
    std::cout<<"inicia vectores"<<std::endl;
    //jcmd_left.names[0]="left_s0";
    //jcmd_left.names[1]="left_s1";
    //jcmd_left.names[2]="left_e0";
    //jcmd_left.names[3]="left_e1";
    //jcmd_left.names[4]="left_w0";
    //jcmd_left.names[5]="left_w1";
    //jcmd_left.names[6]="left_w2";
    
    jcmd_right.names[0]="right_s0";
    jcmd_right.names[1]="right_s1";
    jcmd_right.names[2]="right_e0";
    jcmd_right.names[3]="right_e1";
    jcmd_right.names[4]="right_w0";
    jcmd_right.names[5]="right_w1";
    jcmd_right.names[6]="right_w2";
    
    //jcmd.name.resize(19);
    //jcmd.position.resize(19);
    //jcmd.velocity.resize(19);
    //jcmd.effort.resize(19);
    
    

    //for(int j=0; j<19;j++){
        
    //    jcmd.position[j]=0.0;
        
   // }

    for(int j=0; j<7;j++){
        
        jcmd_right.command[j]=0.0;
        //jcmd_left.command[j]=0.0;
        
    }
    
    //jcmd_left.command[0]=-0.8;
    //jcmd_left.command[1]=-0.7;

    jcmd_right.command[0]=-1.3;
    jcmd_right.command[1]=+1.3;
    
    for (int wait0=0;wait0<200000;wait0++){
    std::cout<<wait0<<std::endl;
    pub_right.publish(jcmd_right);
    }

    jcmd_right.command[0]=0.8;
    jcmd_right.command[1]=-1.4;
    
    
    //pub.publish(jcmd);
    //std::cout<<"jcmd ready"<<std::endl;
    //pub_right.publish(jcmd_right);
    std::cout<<"jcmd_right ready"<<std::endl;
    //pub_left.publish(jcmd_left);
    //std::cout<<"jcmd_left ready"<<std::endl;
    //############################################
    // Tasks and Inverse Kinematics Solver
    //############################################
    unsigned int f = 10000;
    double dt = static_cast<double>(1.0/f);
    KineSolverWQP solver_right(robot, qsensed_right, dt);
    //KineSolverWQP solver_left(robot, qsensed_left, dt);

    solver_right.setJointLimits(qmin, qmax, dqmax);
    //solver_left.setJointLimits(qmin, qmax, dqmax);
    

    KineTask* taskrh = new KineTaskPose(robot, 14, "pose");
    //KineTask* tasklh = new KineTaskPose(robot, 7, "pose");
    

    taskrh->setGain(800.0);
   //tasklh->setGain(800.0);
    
    Eigen::VectorXd qdes_right ;
    //Eigen::VectorXd qdes_left;


    ros::Rate rate(f); // Hz

    //BallMarker* b_hand_right;
    //BallMarker* b_hand_left;
    
    //b_hand_right= new BallMarker(nh, red);
    //b_hand_left= new BallMarker(nh, blue);
    
    
    solver_right.pushTask(taskrh);
    //solver_left.pushTask(tasklh);
    
    //############################################
    //ROS
    //############################################

    
    double k=1;
    double hand_position=0;
    double hola=0;
    
    
        

    while(ros::ok())
    {
        double x_b=0.78;
        double y_b=-0.14;
        double z_b=0.43;
        
        std::cout<<"Right_Point_recibed_x:"<<right_position_baxter.getData()->x<<std::endl;
        std::cout<<"Right_Point_recibed_y:"<<right_position_baxter.getData()->y<<std::endl;
        std::cout<<"Right_Point_recibed_z:"<<right_position_baxter.getData()->z<<std::endl;

                 
        if(right_position_baxter.getData()->x>0.1){                     
            P_right_wrist.resize(6);

            if(right_position_baxter.getData()->x==1000){
            P_right_wrist[0]=x_b;//position_baxter.getData()->x;
            P_right_wrist[1]=y_b;//position_baxter.getData()->y;
            P_right_wrist[2]=z_b;//position_baxter.getData()->z;
             P_right_wrist[3]=1.20;//1.57;reglg
            P_right_wrist[4]=0;
            P_right_wrist[5]=-1.57;//-1.57;De arriba a abajo   
            }
 
            if (right_position_baxter.getData()->x==50){
            P_right_wrist[0]= 0.60;
            P_right_wrist[1]= -0.40;
            P_right_wrist[2]= 0.50;
            P_right_wrist[3]=1.57;//1.57;
            P_right_wrist[4]=0;
            P_right_wrist[5]=-1.57;//-1.57;De arriba a abajo
            }

            if (right_position_baxter.getData()->x<5) {
            P_right_wrist[0]=right_position_baxter.getData()->x;//0.85;//position_baxter.getData()->x;
            P_right_wrist[1]=right_position_baxter.getData()->y;//0.1;//position_baxter.getData()->y;
            P_right_wrist[2]=right_position_baxter.getData()->z;//0.20;//position_baxter.getData()->z;
            P_right_wrist[3]=1.57;//1.57;
            P_right_wrist[4]=0;
            P_right_wrist[5]=-1.57;//-1.57;De arriba a abajo

            }
           
            std::cout<<"set"<<std::endl;
            taskrh->setDesiredValue(P_right_wrist);
            solver_right.getPositionControl(qsensed_right, qdes_right);
            
            ball_position(0,5)=P_right_wrist[0];//position_baxter.getData()->x;
            ball_position(1,5)=P_right_wrist[1];//position_baxter.getData()->y;
            ball_position(2,5)=P_right_wrist[2];//position_baxter.getData()->z;
            
            for(int l=0; l<7;l++)
            {
                jcmd_right.command[l]=qdes_right[l+8];
            }
            

            qsensed_right = qdes_right;
            
            
        }
         
         
        pub_right.publish(jcmd_right);
        //pub_left.publish(jcmd_left);


        ros::spinOnce();
        rate.sleep();
        
        
    }


    return 0;


}