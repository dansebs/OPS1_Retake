int opt;
int times = 1;
char *name = "Default";

while ((opt = getopt(argc, argv, "n:t:")) != -1) {
    switch (opt) {
        case 'n':
            name = optarg;
            break;
        case 't':
            times = atoi(optarg);
            if (times <= 0) usage(argv[0], "-t requires a positive integer");
            break;
        default:
            usage(argv[0], "[-n name] [-t times]");
    }
}