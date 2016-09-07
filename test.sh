#!/bin/bash

./ale -display_screen false \
    -restricted_action_set false \
    -discount_factor 0.995 \
    -randomize_successor_novelty true \
    -max_sim_steps_per_frame 10000 \
    -max_num_frames_per_episode 12000 \
    -max_num_episodes 1 \
    -random_seed 0 \
    -player_agent search_agent \
    -search_method piw1 \
    -action_sequence_detection true \
    -probablistic_action_selection false \
    -junk_decision_frame 2 \
    -longest_junk_sequence 2 \
    -asd_epsilon 0.05 \
    ./roms/atlantis.bin
#    ./roms/up_n_down.bin
#    ./roms/asterix.bin
#    -depth_normalized_reward false \
#    -junk_decision_frame 2 \
#    -decision_frame_function 2 \
#    ./roms/video_pinball.bin
#    ./roms/space_invaders.bin
#    ./roms/crazy_climber.bin
#    ./roms/ms_pacman.bin

#    ./supported_roms/group_3/asterix.bin #  ./supported_roms/group_3/space_invaders.bin #    ./supported_roms/group_2/ms_pacman.bin
