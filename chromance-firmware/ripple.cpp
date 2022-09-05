#include "ripple.h"


float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

Ripple::Ripple(int id) : rippleId(id) {
    Serial.print("Instanced ripple #");
    Serial.println(rippleId);
}

void Ripple::start(byte n, byte d, unsigned long c, float s, unsigned long l, rippleBehavior b) {
    color = c;
    speed = s;
    lifespan = l;
    behavior = b;

    birthday = millis();
    pressure = 0;
    state = STATE_WITHIN_NODE;

    position[0] = n;
    position[1] = d;

    justStarted = true;

    Serial.print("Ripple ");
    Serial.print(rippleId);
    Serial.print(" starting at node ");
    Serial.print(position[0]);
    Serial.print(" direction ");
    Serial.println(position[1]);
}

void Ripple::renderLed(byte ledColors[NUMBER_OF_SEGMENTS][LEDS_PER_SEGMENTS][3], unsigned long age) {
    int strip = ledAssignments[position[0]][0];
    int led = ledAssignments[position[0]][2] + position[1];
    int red = ledColors[position[0]][position[1]][0];
    int green = ledColors[position[0]][position[1]][1];
    int blue = ledColors[position[0]][position[1]][2];

    ledColors[position[0]][position[1]][0] = byte(min(255, max(0, int(fmap(float(age), 0.0, float(lifespan), (color >> 8) & 0xFF, 0.0)) + red)));
    ledColors[position[0]][position[1]][1] = byte(min(255, max(0, int(fmap(float(age), 0.0, float(lifespan), (color >> 16) & 0xFF, 0.0)) + green)));
    ledColors[position[0]][position[1]][2] = byte(min(255, max(0, int(fmap(float(age), 0.0, float(lifespan), color & 0xFF, 0.0)) + blue)));
}

void Ripple::advance(byte ledColors[NUMBER_OF_SEGMENTS][LEDS_PER_SEGMENTS][3]) {
    
    unsigned long age = millis() - birthday;

    if (state == STATE_DEAD) {
        return;            
    }

    pressure += fmap(float(age), 0.0, float(lifespan), speed, 0.0);  // Ripple slows down as it ages
    // TODO: Motion of ripple is severely affected by loop speed. Make it time invariant

    if (pressure < 1 && (state == STATE_TRAVEL_UP || state == STATE_TRAVEL_DOWN)) {
        // Ripple is visible but hasn't moved - render it to avoid flickering
        renderLed(ledColors, age);
    }

    while (pressure >= 1) {
#ifdef DEBUG_ADVANCEMENT
        Serial.print("---- Ripple ");
        Serial.print(rippleId);
        Serial.println(" advancing:");
#endif

        switch (state) { 
            case STATE_WITHIN_NODE: {
                if (justStarted) {
                    justStarted = false;
                }
                else {
#ifdef DEBUG_ADVANCEMENT
                    Serial.print("  Picking direction out of node ");
                    Serial.print(position[0]);
                    Serial.print(" with agr. ");
                    Serial.println(behavior);
#endif

                    int newDirection = -1;

                    int sharpLeft = (position[1] + 1) % SIDES_PER_NODES;
                    int wideLeft = (position[1] + 2) % SIDES_PER_NODES;
                    int forward = (position[1] + 3) % SIDES_PER_NODES;
                    int wideRight = (position[1] + 4) % SIDES_PER_NODES;
                    int sharpRight = (position[1] + 5) % SIDES_PER_NODES;

                    if (behavior <= BEHAVIOR_ANGRY) {  // Semi-random aggressive turn mode
                        // The more aggressive a ripple, the tighter turns it wants to make.
                        // If there aren't any segments it can turn to, we need to adjust its behavior.
                        byte anger = behavior;
                        int forwardConnection = nodeConnections[position[0]][forward];

                        while (newDirection < 0) {
                            if (anger == BEHAVIOR_COUCH_POTATO) {

                                // We can't go straight ahead - we need to take a rest
#ifdef DEBUG_ADVANCEMENT
                                Serial.println("  Never continue ... too lazy - stopping");
#endif
                                // Die now
                                age = lifespan;
                                break;
                            }

                            if (anger == BEHAVIOR_LAZY) {

                                if (forwardConnection < 0) {
                                    // We can't go straight ahead - we need to take a rest
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Can't go straight - stopping");
            #endif
                                    // Die now
                                    age = lifespan;
                                    break;
                                    //break;
                                }
                                else {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Going forward");
            #endif
                                    newDirection = forward;
                                }
                            }

                            if (anger == BEHAVIOR_WEAK) {

                                if (forwardConnection < 0) {
                                    // We can't go straight ahead - we need to take a more aggressive angle
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Can't go straight - picking more agr. path");
            #endif
                                    anger += 1;
                                }
                                else {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Going forward");
            #endif
                                    newDirection = forward;
                                }
                            }

                            if (anger == BEHAVIOR_FEISTY) {
                                int leftConnection = nodeConnections[position[0]][wideLeft];
                                int rightConnection = nodeConnections[position[0]][wideRight];

                                if (leftConnection >= 0 && rightConnection >= 0) {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Turning left or right at random");
            #endif
                                    newDirection = random(2) ? wideLeft : wideRight;
                                }
                                else if (leftConnection >= 0) {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Can only turn left");
            #endif
                                    newDirection = wideLeft;
                                }
                                else if (rightConnection >= 0) {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Can only turn right");
            #endif
                                    newDirection = wideRight;
                                }
                                else {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Can't make wide turn - picking more agr. path");
            #endif
                                    anger+=1;  // Can't take shallow turn - must become more aggressive
                                }
                            }

                            if (anger == BEHAVIOR_ANGRY) {
                                int leftConnection = nodeConnections[position[0]][sharpLeft];
                                int rightConnection = nodeConnections[position[0]][sharpRight];

                                if (leftConnection >= 0 && rightConnection >= 0) {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Turning left or right at random");
            #endif
                                    newDirection = random(2) ? sharpLeft : sharpRight;
                                }
                                else if (leftConnection >= 0) {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Can only turn left");
            #endif
                                    newDirection = sharpLeft;
                                }
                                else if (rightConnection >= 0) {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Can only turn right");
            #endif
                                    newDirection = sharpRight;
                                }
                                else {
            #ifdef DEBUG_ADVANCEMENT
                                    Serial.println("  Can't make tight turn - picking less agr. path");
            #endif
                                    anger--;  // Can't take tight turn - must become less aggressive
                                }
                            }

                            // Note that this can't handle some circumstances,
                            // like a node with segments in nothing but the 0 and 3 positions.
                            // Good thing we don't have any of those!

                        } // End while loop
                    }
                    else if (behavior == BEHAVIOR_ALWAYS_RIGHT) {
                        for (int i = 1; i < SIDES_PER_NODES; i++) {
                            int possibleDirection = (position[1] + i) % SIDES_PER_NODES;

                            if (nodeConnections[position[0]][possibleDirection] >= 0) {
                                newDirection = possibleDirection;
                                break;
                            }
                        }
        #ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Turning as rightward as possible");
        #endif
                    }
                    else if (behavior == BEHAVIOR_ALWAYS_LEFT) {
                        for (int i = 5; i >= 1; i--) {
                            int possibleDirection = (position[1] + i) % SIDES_PER_NODES;

                            if (nodeConnections[position[0]][possibleDirection] >= 0) {
                                newDirection = possibleDirection;
                                break;
                            }
                        }
        #ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Turning as leftward as possible");
        #endif
                    }
                    else if (behavior == BEHAVIOR_EXPLODING) {
                        for (int i = 5; i >= 1; i--) {
                            int possibleDirection = (position[1] + i) % SIDES_PER_NODES;

                            if (nodeConnections[position[0]][possibleDirection] >= 0 && (possibleDirection != position[0])) {
                                newDirection = possibleDirection;
                            // start
                            }
                        }

#ifdef DEBUG_ADVANCEMENT
                            Serial.println(" Exploding !");
#endif
                    }

#ifdef DEBUG_ADVANCEMENT
                    Serial.print("  Leaving node ");
                    Serial.print(position[0]);
                    Serial.print(" in direction ");
                    Serial.println(newDirection);
#endif
                    if (newDirection >= 0){
                        position[1] = newDirection;
                    }
                }// End else from 'if (justStarted) {'

                position[0] = nodeConnections[position[0]][position[1]];  // Look up which segment we're on
                if ( position[0] > 37 )
                {
                    Serial.print("Uhoh, position[0] out of bound at line 296 :");
                    Serial.println(position[0]);
                }
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  and entering segment ");
                Serial.println(position[0]);
#endif

                if (position[1] == 5 || position[1] == 0 || position[1] == 1) {  // Top half of the node
#ifdef DEBUG_ADVANCEMENT
                    Serial.println("  (starting at bottom)");
#endif
                    state = STATE_TRAVEL_UP;
                    position[1] = 0;  // Starting at bottom of segment
                }
                else {
#ifdef DEBUG_ADVANCEMENT
                    Serial.println("  (starting at top)");
#endif
                    state = STATE_TRAVEL_DOWN;
                    position[1] = LEDS_PER_SEGMENTS - 1; // Starting at top of LED-strip
                }
                break;
            }
            
            case STATE_TRAVEL_UP: {
                position[1]++;

                if (position[1] >= LEDS_PER_SEGMENTS) {
                    // We've reached the top!
#ifdef DEBUG_ADVANCEMENT
                    Serial.print("  Reached top of seg. ");
                    Serial.println(position[0]);
#endif
                    // Enter the new node.
                    int segment = position[0];
                    position[0] = segmentConnections[position[0]][0];
                    if ( position[0] > NUMBER_OF_SEGMENTS )
                    {
                    Serial.print("Segment out of bound :");
                    Serial.print(position[0]);
                    Serial.println("");
                    }
                    for (int i = 0; i < SIDES_PER_NODES; i++) {
                    // Figure out from which direction the ripple is entering the node.
                    // Allows us to exit in an appropriately aggressive direction.
                    int incomingConnection = nodeConnections[position[0]][i];
                    if (incomingConnection == segment)
                        position[1] = i;
                    }
#ifdef DEBUG_ADVANCEMENT
                    Serial.print("  Entering node ");
                    Serial.print(position[0]);
                    Serial.print(" from direction ");
                    Serial.println(position[1]);
#endif
                    state = STATE_WITHIN_NODE;
                }
                else {
#ifdef DEBUG_ADVANCEMENT
                    Serial.print("  Moved up to seg. ");
                    Serial.print(position[0]);
                    Serial.print(" LED ");
                    Serial.println(position[1]);
#endif
                }
                break;
            }

            case STATE_TRAVEL_DOWN: {
                position[1]--;
                if (position[1] < 0) {
                    // We've reached the bottom!
#ifdef DEBUG_ADVANCEMENT
                    Serial.print("  Reached bottom of seg. ");
                    Serial.println(position[0]);
#endif
                    // Enter the new node.
                    int segment = position[0];
                    position[0] = segmentConnections[position[0]][1];
                    {
                    Serial.print("Segment out of bound :");
                    Serial.print(position[0]);
                    Serial.println("");
                    }
                    for (int i = 0; i < 6; i++) {
                    // Figure out from which direction the ripple is entering the node.
                    // Allows us to exit in an appropriately aggressive direction.
                    int incomingConnection = nodeConnections[position[0]][i];
                    if (incomingConnection == segment)
                        position[1] = i;
                    }
#ifdef DEBUG_ADVANCEMENT
                    Serial.print("  Entering node ");
                    Serial.print(position[0]);
                    Serial.print(" from direction ");
                    Serial.println(position[1]);
#endif
                    state = STATE_WITHIN_NODE;
                }
                else {
#ifdef DEBUG_ADVANCEMENT
                    Serial.print("  Moved down to seg. ");
                    Serial.print(position[0]);
                    Serial.print(" LED ");
                    Serial.println(position[1]);
#endif
                }
                break;
            }

        default:
        break;
    }

    pressure -= 1;

    if (state == STATE_TRAVEL_UP || state == STATE_TRAVEL_DOWN) {
        // Ripple is visible - render it
        renderLed(ledColors, age);
    }
    }

#ifdef DEBUG_AGE
    Serial.print("  Age is now ");
    Serial.print(age);
    Serial.print('/');
    Serial.println(lifespan);
#endif

    if (lifespan && age >= lifespan) {
    // We STATE_DEAD
#ifdef DEBUG_AGE
    Serial.println("  Lifespan is up! Ripple is STATE_DEAD.");
#endif
    state = STATE_DEAD;
    position[0] = position[1] = pressure = age = 0;
    }

#ifdef DEBUG_RENDERING
        Serial.print("Rendering ripple position (");
        Serial.print(position[0]);
        Serial.print(',');
        Serial.print(position[1]);
        Serial.print(") at Strip ");
        Serial.print(strip);
        Serial.print(", LED ");
        Serial.print(led);
        Serial.print(", color 0x");
        for (int i = 0; i < 3; i++) {
        if (ledColors[position[0]][position[1]][i] <= 0x0F){
            Serial.print('0');
        }
        Serial.print(ledColors[position[0]][position[1]][i], HEX);
        }
        Serial.println();
#endif
}