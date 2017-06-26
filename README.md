# Dominated Action Sequence Detection for Blind Search 

This is a code for Dominated Action Sequence Detection/Avoidance presented in:

Jinnai Y, Fukunaga A. 2017. Learning to Prune Dominated Action Sequences in Online Black-box Planning. Proc. 31st AAAI Conference on Artificial Intelligence (AAAI-17)

The code extends the work of Lipovetzky et al. (2015) (https://github.com/miquelramirez/ALE-Atari-Width).
In addition to the original code we implemented p-IW1 which is presented in:

Shleyfman A, Tuisov A, Domshlak C. 2016. Blind Search for Atari-Like Online Planning Revisited. Proc. 25th International Joint Conference on Artificial Intelligence (IJCAI-16)

Note that this code is based on Arcade Learning Environment (ALE) version 0.4.


Arcade-Learning-Environment
===========================

The Arcade Learning Environment (ALE) -- a platform for AI research.

See http://www.arcadelearningenvironment.org for details.

Classical Planning algorithms
=============================

The Classical Planning algorithms are located in: 

  IW(1)
  
      * src/agents/IW1Search.hpp
      * src/agents/IW1Search.cpp

  p-IW(1)
  
      * src/agents/PIW1Search.hpp
      * src/agents/PIW1Search.cpp

  Dominated Action Sequence Detection/Avoidance
  
      * src/agents/DominatedActionSequenceDetection.hpp
      * src/agents/DominatedActionSequenceDetection.cpp
      * src/agents/DominatedActionSequenceAvoidance.hpp
      * src/agents/DominatedActionSequenceAvoidance.cpp

  Brute (Bellemare et al 2013)
 
      * src/agents/Brute.hpp
      * src/agents/Brute.cpp

  2BFS

      * src/agents/BestFirstSearch.hpp
      * src/agents/BestFirstSearch.cpp

To compile, rename the makefile either for linux or mac

RUNNING ALE
===========

The command to run IW1 is 

```
      ./ale -display_screen true -discount_factor 0.995 -randomize_successor_novelty true -max_sim_steps_per_frame 150000  -player_agent search_agent -search_method iw1  (ROM_PATH)
```

The command to run p-IW1 is 

```
      ./ale -display_screen true -discount_factor 0.995 -randomize_successor_novelty true -max_sim_steps_per_frame 150000  -player_agent search_agent -search_method piw1  (ROM_PATH)
```

The command to run 2BFS is 
```
      ./ale -display_screen true -discount_factor 0.995 -randomize_successor_novelty true -max_sim_steps_per_frame 150000  -player_agent search_agent -search_method bfs  (ROM_PATH)
```

The command to run IW1 with Dominated Action Sequence Detection is 

```
      ./ale -display_screen true -discount_factor 0.995 -randomize_successor_novelty true -max_sim_steps_per_frame 150000  -player_agent search_agent -search_method piw1 
-action_sequence_detection true -longest_junk_sequence 1 (ROM_PATH)
```

and you have to substitute ROM_PATH for any of the games under *supported_roms* folder.

*max_sim_steps_per_frame* sets your budget, i.e. how many frames you can expand per lookahead. Each node is 5 frames of game play, so 150,000, is equivalent to 30,000 nodes generated. You don't need that many to find rewards, so you can use a smaller number to play the game much faster. 150,000 was the parameter chosen by Bellemare et al. The most expensive computation is calling the simulator to generate the successor state.



Python scripts
==============

are the ones we wrote to make life easier for experimentation:

    * Evaluate_agents.py may help you to run experiments.

We wrote some code to record the games, add the following flag to the ./ale command 

   -record_trajectory true

and then you can replay the game using

    * replay.py <state_trajectory_alg_game_episode.i file>
    
Credits
=======

We want to thank Marc Bellemare for making the ALE code available and the research group at U. Alberta.

The Classical Planning algorithms code is adapted from the Lightweight Automated Planning Toolkit (www.LAPKT.org)

