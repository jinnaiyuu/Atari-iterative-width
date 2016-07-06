#!/bin/bash

./ale -display_screen true \
 -discount_factor 0.995 -randomize_successor_novelty true -max_sim_steps_per_frame 10000 -max_num_frames_per_episode 20 -max_num_episodes 1 -player_agent search_agent -search_method brfs  ./supported_roms/group_1/atlantis.bin
