#!/bin/bash

./ale -display_screen false \
    -discount_factor 0.995 \
    -randomize_successor_novelty true \
    -max_sim_steps_per_frame 5000 \
    -max_num_frames_per_episode 1200 \
    -max_num_episodes 1 \
    -random_seed 0 \
    -player_agent search_agent \
    -search_method uct \
    -restricted_action_set true \
    ./roms/alien.bin

exit 
