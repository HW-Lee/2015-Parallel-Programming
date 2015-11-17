import sys
import os
import numpy as np
import numpy.matlib

def main(n):
	num_r = np.sqrt(n) / 2;
	if num_r < 10: num_r = 5
	elif num_r < 20: num_r = 20
	elif num_r < 25: num_r = 25
	elif num_r < 50: num_r = 50
	else: num_r = 100

	rr = np.linspace(1.0/num_r, 1.0, num_r)
	rads = np.linspace(0.0, 2*np.pi, n/num_r)

	rr = np.matlib.repmat(rr, n/num_r, 1)
	rads = np.matlib.repmat(rads, num_r, 1)

	grid = np.zeros([n, 4])
	grid[:, 0] = rr.flatten()
	grid[:, 1] = rads.transpose().flatten()
	grid[:, [2, 3]] = 0.1 * np.array( map(tangent_vec, grid[:, 1]) )
	grid[:, [0, 1]] = np.array( map(polar2card, grid[:, [0, 1]]) )

	with open("gen_" + str(n) + ".txt", "w") as out_file:
		out_file.write(str(n) + "\n")
		map(lambda x: out_file.write(" ".join(map(str, x)) + "\n"), grid)


def polar2card(pt):
	return pt[0] * np.array([np.cos(pt[1]), np.sin(pt[1])])

def tangent_vec(rad):
	return np.array([-np.sin(rad), np.cos(rad)])

if __name__ == "__main__": main(int(sys.argv[1]))
