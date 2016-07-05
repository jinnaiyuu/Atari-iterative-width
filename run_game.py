#!/usr/bin/python

import os
import sys

def main() :

	if len(sys.argv) < 3 :
		print >> sys.stderr, "Missing parameters!"
		print >> sys.stderr, "Usage: ./run_game.py <search_method> <path to ROM>"
		sys.exit(1)

	search_method = sys.argv[1]
	rom_path = sys.argv[2]

	if not os.path.exists( rom_path ) :
		print >> sys.stderr, "Could not find ROM:", rom_path
		sys.exit(1)


	command = './ale -display_screen true -max_sim_steps_per_frame 200 -player_agent search_agent -search_method %(search_method)s %(rom_path)s'%locals()
	print >> sys.stdout, "Command:"
	print >> sys.stdout, command
	os.system( command)

if __name__ == '__main__' :
	main()
