import os

def main():
    
    usr = "s103061527"

    cmd = "ssh " + usr + "@" + "140.114.91.171"
    os.system( cmd )


if __name__ == '__main__': main()