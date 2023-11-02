void puzzle_init(void){
  //Declare pins etc
}

void puzzle_loop(void){
  checkSolution();
}

void checkSolution(void){
  for(int i=0; i<NUM_PUZZLES; i++){
    if(game_state[i] == atoi(PUZZLE_SOLVED) && !(flag_solved[i])){
      mqttClient.publish(state_topic[i], 1, true, PUZZLE_SOLVED);
      flag_solved[i] = true;
      //digitalWrite(OUTPUT_1, LOW);
      Serial.println(state_message[2*i]);
    }
    else if(game_state[i] == atoi(PUZZLE_READY) && flag_solved[i]){
      mqttClient.publish(state_topic[i], 1, true, PUZZLE_READY);
      flag_solved[i] = false;
      //digitalWrite(OUTPUT_1, LOW);
      Serial.println(state_message[2*i + 1]);
    }
  }
}

void reset_loop(void){
  // Open all locks and doors until callback disables this loop
  for(int i=0; i<NUM_PUZZLES; i++){
    
  }
}
