#include <Arduino_FreeRTOS.h>

void Check_Sensor(void *param) {
  //code for checking sensor
  while(1)

  {
    Serial.println("Checking Sensor");
    vTaskDelay(1000/portTICK_PERIOD_MS); //prints every second
  }
}

void Take_Picture(void *param) {
  //take picture
}

void Locate(void *param) {
  //use gps to locate the CanSat
}

void Store_SD(void *param) {
  //store memory of picture being taken
}

void Lora_Communication(void *param) {
  //send info to LorA module
}

void Velocity(void *param){
  //measure accelerometer to determine deployment
}

void Parachute_Deploy(void *param) {
  //parachute deployment
}

TaskHandle_t Check_Sensor_Handle;
TaskHandle_t Lora_Communication_Handle;
TaskHandle_t Velocity_Handle;
TaskHandle_t Parachute_Deploy_Handle;
TaskHandle_t Store_SD_Handle;
TaskHandle_t Take_Picture_Handle;
TaskHandle_t Locate_Handle;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  xTaskCreate(Check_Sensor, "CheckSensor", 100, NULL, 1, &Check_Sensor_Handle)

}

void loop() {
  // put your main code here, to run repeatedly:

}
