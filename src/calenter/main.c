int calenter_main();
int cli_main(int argc, char** argv);

int main(int argc, char** argv) {
    if (argc == 1)
        return calenter_main();

    return cli_main(argc, argv);
}
