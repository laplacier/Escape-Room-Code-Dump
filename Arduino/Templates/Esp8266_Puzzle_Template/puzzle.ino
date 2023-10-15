void puzzle_init(void){
  //Declare pins etc
}

void puzzle_loop(void){
  checkSolution();
}

void checkSolution(void){
  for(int i=0; i<NUM_PUZZLES; i++){
    if(isSolved[i] && !(flag_solved[i])){
      mqttClient.publish(state_topic[i], 1, true, "true");
      flag_solved[i] = true;
      //digitalWrite(OUTPUT_1, LOW);
      Serial.println(state_message[2*i]);
    }
    else if(!(isSolved[i]) && flag_solved[i]){
      mqttClient.publish(state_topic[i], 1, true, "false");
      flag_solved[i] = false;
      //digitalWrite(OUTPUT_1, LOW);
      Serial.println(state_message[2*i + 1]);
    }
  }
}
