generic configuration PositionSensorC() 
{
	provides interface Read<sensor_status>;
} 

implementation 
{

	components MainC, RandomC;
	components new PositionSensorP();
	
	//Connects the provided interface
	Read = PositionSensorP;
	
	//Random interface and its initialization	
	PositionSensorP.Random -> RandomC;
	RandomC <- MainC.SoftwareInit;

}
