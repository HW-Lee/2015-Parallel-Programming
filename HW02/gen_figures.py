import sys
import os
import json
import numpy as np
import matplotlib.pyplot as pyplot

def main():
	with open("exp.json") as data_file:
		data = json.load(data_file)

	nthreads_data = data["nthreads_var"]
	nsteps_data = data["nsteps_var"]
	nbodies_data = data["nbodies_var"]
	theta_data = data["theta_var"]

	gen_figure(nthreads_data, "nthreads")
	gen_figure(nsteps_data, "nsteps")
	gen_figure(nbodies_data, "nbodies")
	gen_figure(theta_data, "theta")


def gen_figure(data, name):
	cmd = data["command"]

	if "pthread" in data.keys():

		x = map(lambda x: x[name], data["pthread"])
		y_pthread = map(lambda x: x["elapsed"], data["pthread"])
		y_openmp = map(lambda x: x["elapsed"], data["openmp"])
		y_bhalgo = map(lambda x: (x["theta"], map(lambda y: y["elapsed"], x["data"])), data["bhalgo"])
		pyplot.plot(x, y_pthread, "sr-", label="pthread")
		pyplot.plot(x, y_openmp, "sb-", label="openmp")

		color_arr = ["g", "y"]
		idx = 0

		y_max = max(max(y_pthread), max(y_openmp))

		for each in y_bhalgo:
			theta, y_bhalgo_dummy = each
			y_bhalgo_dummy = map(lambda x: sum(x), y_bhalgo_dummy)
			pyplot.plot(x, y_bhalgo_dummy, "s" + color_arr[idx] + "-", label="Barnes-Hut\ntheta = " + str(theta))
			idx += 1
			y_max = max(y_max, max(y_bhalgo_dummy))

		if y_max > max(y_pthread[0], y_openmp[0]): pyplot.ylim(0, y_max*1.3)
		pyplot.xlabel(name)

	else:

		x = map(lambda x: x["theta"], data["data"])
		y = np.array(map(lambda x: np.array(x["elapsed"]), data["data"]))
		pyplot.bar(np.arange(len(x)), y[:, 1], color="g", label="tree-build", align="center")
		pyplot.bar(np.arange(len(x)), y[:, 0], color="b", label="force-compute", align="center", bottom=y[:, 1])
		pyplot.bar([0], [0], color="r", label="I/O", align="center")
		pyplot.xticks(np.arange(len(x)), x)
		pyplot.xlabel("theta")


	pyplot.ylabel("time(ms)")
	pyplot.legend()
	pyplot.title("command: " + cmd)

	pyplot.gcf().set_size_inches(12, 9)
	pyplot.savefig("./report/" + name + ".png", transparent=True, bbox_inches="tight", pad_inches=0.1)
	pyplot.gcf().clear()

if __name__ == "__main__": main()
