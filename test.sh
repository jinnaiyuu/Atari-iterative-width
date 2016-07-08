#!/bin/bash

./ale -display_screen true \
 -discount_factor 0.995 -randomize_successor_novelty true -max_sim_steps_per_frame 2000 -max_num_frames_per_episode 120 -max_num_episodes 1 -player_agent search_agent -search_method piw1  ./supported_roms/group_1/atlantis.bin #    ./supported_roms/group_2/ms_pacman.bin
