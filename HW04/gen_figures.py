import sys
import os
import json
import numpy as np
import matplotlib.pyplot as pyplot

def main():
    with open("exp.json") as data_file:
        data = json.load(data_file)

    data_ws = data["weak-scalibility"]
    data_bf = data["blocking-factor"]

    gen_ws_time_figure(data_ws)
    gen_ws_gflops_figure(data_ws)

    gen_bf_time_figure(data_bf)
    gen_bf_gflops_figure(data_bf)
    gen_bw_figure(data_bf)


def gen_ws_time_figure(data):
    cmd = data["command"]
    single_info = data["single"]
    multi_info = data["multi"]

    single_details = map(lambda x: x["detail"][0]["elapsed"], single_info)
    single_sizes = map(lambda x: x["size"], single_info)
    multi_details = map(lambda x: x["detail"][0]["elapsed"], multi_info)
    multi_sizes = map(lambda x: x["size"], multi_info)

    elapsed_keys = ["updateList", "[CUDA memcpy HtoD]", "[CUDA memcpy DtoH]"]
    single_elapsed = np.array(map(lambda key: map(lambda x: x[key], single_details), elapsed_keys))
    multi_elapsed = np.array(map(lambda key: map(lambda x: x[key], multi_details), elapsed_keys))

    width = 0.15
    x = np.arange(len(single_sizes)) + 1

    colors = ["b", "r", "g"]
    btm = np.zeros(np.array(single_sizes).shape)
    for i in range(len(elapsed_keys)):
        pyplot.bar(x - width, single_elapsed[i, :], width,
            color="white",
            edgecolor=colors[i],
            label=elapsed_keys[i] + "(single)",
            align="center",
            bottom=btm,
            hatch="/")
        btm = btm + single_elapsed[i, :]

    btm = np.zeros(np.array(single_sizes).shape)
    for i in range(len(elapsed_keys)):
        pyplot.bar(x + width, multi_elapsed[i, :], width,
            color="white",
            edgecolor=colors[i],
            label=elapsed_keys[i] + "(multi)",
            align="center",
            bottom=btm,
            hatch="x")
        btm = btm + multi_elapsed[i, :]

    pyplot.xticks(x, single_sizes)
    pyplot.ylim(0, 35000)
    pyplot.legend()
    pyplot.title("Weak Scalibility in terms of execution time")
    pyplot.xlabel("# of vertices")
    pyplot.ylabel("Time (ms)")
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/weak-scalibility-time.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

def gen_ws_gflops_figure(data):
    cmd = data["command"]
    single_info = data["single"]
    multi_info = data["multi"]

    single_details = map(lambda x: x["detail"][0]["elapsed"], single_info)
    single_sizes = np.array(map(lambda x: x["size"], single_info))
    multi_details = map(lambda x: x["detail"][0]["elapsed"], multi_info)
    multi_sizes = np.array(map(lambda x: x["size"], multi_info))

    elapsed_keys = ["updateList", "[CUDA memcpy HtoD]", "[CUDA memcpy DtoH]"]
    single_tot_elapsed = map(lambda key: np.array(map(lambda x: x[key], single_details)), elapsed_keys)
    multi_tot_elapsed = map(lambda key: np.array(map(lambda x: x[key], multi_details)), elapsed_keys)

    single_tot_elapsed = np.array(reduce(lambda x, y: x + y, single_tot_elapsed))
    multi_tot_elapsed = np.array(reduce(lambda x, y: x + y, multi_tot_elapsed))

    width = 0.15
    x = np.arange(len(single_sizes)) + 1

    pyplot.bar(x - width, 4000 * np.divide(np.power(single_sizes, 3) / 1024.**3, single_tot_elapsed), width,
        color="g",
        label="single",
        align="center")

    pyplot.bar(x + width, 4000 * np.divide(np.power(multi_sizes, 3) / 1024.**3, multi_tot_elapsed), width,
        color="b",
        label="multi",
        align="center")

    pyplot.xticks(x, single_sizes)
    pyplot.ylim(0, 400)
    pyplot.legend()
    pyplot.title("Weak Scalibility in terms of performance")
    pyplot.xlabel("# of vertices")
    pyplot.ylabel("GFLOPS")
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/weak-scalibility-perf.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()


def gen_bf_time_figure(data):
    single_info = data["single"]
    omp_info = data["omp"]
    mpi_info = data["mpi"]

    single_details = map(lambda x: x["detail"][0]["elapsed"], single_info)
    single_factors = map(lambda x: x["factor"], single_info)
    omp_details = map(lambda x: x["detail"][0]["elapsed"], omp_info)
    mpi_details = map(lambda x: x["detail"][0]["elapsed"], mpi_info)

    elapsed_keys = ["updateList", "[CUDA memcpy HtoD]", "[CUDA memcpy DtoH]"]
    single_elapsed = np.array(map(lambda key: map(lambda x: x[key], single_details), elapsed_keys))
    omp_elapsed = np.array(map(lambda key: map(lambda x: x[key], omp_details), elapsed_keys))
    mpi_elapsed = np.array(map(lambda key: map(lambda x: x[key], mpi_details), elapsed_keys))

    width = 0.15
    x = np.arange(len(single_factors)) + 1

    colors = ["b", "r", "g"]
    btm = np.zeros(np.array(single_factors).shape)
    for i in range(len(elapsed_keys)):
        pyplot.bar(x - 1.1*width, single_elapsed[i, :], width,
            color="white",
            edgecolor=colors[i],
            label=elapsed_keys[i] + "(single)",
            align="center",
            bottom=btm,
            hatch="/")
        btm = btm + single_elapsed[i, :]

    btm = np.zeros(np.array(single_factors).shape)
    for i in range(len(elapsed_keys)):
        pyplot.bar(x, omp_elapsed[i, :], width,
            color="white",
            edgecolor=colors[i],
            label=elapsed_keys[i] + "(omp)",
            align="center",
            bottom=btm,
            hatch="o")
        btm = btm + omp_elapsed[i, :]

    btm = np.zeros(np.array(single_factors).shape)
    for i in range(len(elapsed_keys)):
        pyplot.bar(x + 1.1*width, mpi_elapsed[i, :], width,
            color="white",
            edgecolor=colors[i],
            label=elapsed_keys[i] + "(mpi)",
            align="center",
            bottom=btm,
            hatch="x")
        btm = btm + mpi_elapsed[i, :]

    pyplot.xticks(x, single_factors)
    pyplot.legend()
    pyplot.title("Execution time with different blocking factors")
    pyplot.xlabel("blocking factor")
    pyplot.ylabel("Time (ms)")
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/blocking-factor-time.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

def gen_bf_gflops_figure(data):
    single_info = data["single"]
    omp_info = data["omp"]
    mpi_info = data["mpi"]

    single_details = map(lambda x: x["detail"][0]["elapsed"], single_info)
    single_factors = np.array(map(lambda x: x["factor"], single_info))
    omp_details = map(lambda x: x["detail"][0]["elapsed"], omp_info)
    mpi_details = map(lambda x: x["detail"][0]["elapsed"], mpi_info)

    elapsed_keys = ["updateList", "[CUDA memcpy HtoD]", "[CUDA memcpy DtoH]"]
    single_tot_elapsed = map(lambda key: np.array(map(lambda x: x[key], single_details)), elapsed_keys)
    omp_tot_elapsed = map(lambda key: np.array(map(lambda x: x[key], omp_details)), elapsed_keys)
    mpi_tot_elapsed = map(lambda key: np.array(map(lambda x: x[key], mpi_details)), elapsed_keys)

    single_tot_elapsed = np.array(reduce(lambda x, y: x + y, single_tot_elapsed))
    omp_tot_elapsed = np.array(reduce(lambda x, y: x + y, omp_tot_elapsed))
    mpi_tot_elapsed = np.array(reduce(lambda x, y: x + y, mpi_tot_elapsed))

    width = 0.15
    x = np.arange(len(single_factors)) + 1

    pyplot.bar(x - 1.1*width, 4000 * np.divide(6000**3 / 1024.**3, single_tot_elapsed), width,
        color="r",
        label="single",
        align="center")

    pyplot.bar(x, 4000 * np.divide(6000**3 / 1024.**3, omp_tot_elapsed), width,
        color="g",
        label="omp",
        align="center")

    pyplot.bar(x + 1.1*width, 4000 * np.divide(6000**3 / 1024.**3, mpi_tot_elapsed), width,
        color="b",
        label="mpi",
        align="center")

    pyplot.xticks(x, single_factors)
    pyplot.legend()
    pyplot.title("Performance with different blocking factors")
    pyplot.xlabel("blocking factor")
    pyplot.ylabel("GFLOPS")
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/blocking-factor-perf.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()

def gen_bw_figure(data):
    omp_info = data["omp"]
    mpi_info = data["mpi"]

    factors = map(lambda x: x["factor"], omp_info)
    omp_bws = map(lambda x: x["mem-bw"], omp_info)
    mpi_bws = map(lambda x: x["mem-bw"], mpi_info)

    x = np.arange(len(factors)) + 1
    pyplot.plot(x, omp_bws, "r-s", label="omp")
    pyplot.plot(x, mpi_bws, "b-s", label="mpi")

    pyplot.xticks(x, factors)
    pyplot.ylim(3, 7)
    pyplot.legend()
    pyplot.title("Memory bandwidth with different blocking factors")
    pyplot.xlabel("blocking factor")
    pyplot.ylabel("GB/s")
    pyplot.gcf().set_size_inches(12, 9)
    pyplot.savefig("./report/mem-bw.png", transparent=True, bbox_inches="tight", pad_inches=0.1)
    pyplot.gcf().clear()


if __name__ == "__main__": main()
