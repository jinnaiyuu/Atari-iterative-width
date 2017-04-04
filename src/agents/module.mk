MODULE := src/agents

MODULE_OBJS := \
	src/agents/bit_array.o \
	src/agents/PlayerAgent.o \
	src/agents/RandomAgent.o \
	src/agents/SingleActionAgent.o \
	src/agents/SDLKeyboardAgent.o \
	src/agents/SimpleBanditAgent.o \
	src/agents/SearchAgent.o \
	src/agents/VertexCover.o \
	src/agents/DominatedActionSequenceAvoidance.o \
	src/agents/DominatedActionSequencePruning.o \
	src/agents/DominatedActionSequenceDetection.o \
	src/agents/SearchTree.o \
	src/agents/TreeNode.o \
	src/agents/FullSearchTree.o \
	src/agents/UCTSearchTree.o \
	src/agents/UCTTreeNode.o \
	src/agents/BreadthFirstSearch.o \
	src/agents/IW1Search.o \
	src/agents/PIW1Search.o \
	src/agents/BestFirstSearch.o \
	src/agents/BondPercolation.o \
	src/agents/SitePercolation.o \
	src/agents/UniformCostSearch.o


MODULE_OBJS += \
	src/agents/features/Features.o \
	src/agents/features/RAMBytes.o \
	src/agents/features/TFBinary.o \
	src/agents/features/ScreenPixels.o \
	src/agents/features/Background.o \
	src/agents/features/BasicFeatures.o \
	src/agents/features/BPROFeatures.o	
	

MODULE_DIRS += \
	src/agents

# Include common rules 
include $(srcdir)/common.rules
