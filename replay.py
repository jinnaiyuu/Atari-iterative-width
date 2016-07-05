#!/usr/bin/python
import sys, os

def main() :
	
	if len(sys.argv) < 3 :
		print >> sys.stderr, "Not enough parameters!"
		print >> sys.stderr, "Usage: ./replay.py <trajectory file> <rom>"
		sys.exit(1)

	trajectory = sys.argv[1]
	rom = sys.argv[2]

	if not os.path.exists( trajectory ) :
		print >> sys.stderr, "Could not find the specified trajectory file", trajectory
		sys.exit(1)

	if not os.path.exists( rom ) :
		print >> sys.stderr, "Could not find the specified game ROM", rom
		sys.exit(1)

	command = './ale -display_screen true -game_controller trajectory -state_trajectory_filename %(trajectory)s %(rom)s'

	os.system( command%locals() )

if __name__ == '__main__' :
	main()
