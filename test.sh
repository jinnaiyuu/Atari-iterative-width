#!/bin/bash

#./ale -display_screen true \
./ale \
 -discount_factor 0.995 -randomize_successor_novelty true -max_sim_steps_per_frame 150000  -player_agent search_agent -search_method piw1  ./supported_roms/group_1/atlantis.bin
