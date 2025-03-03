# dirstats

## Show statistics of current directory
The idea is to get a quick overview of a new codebase to assess the project size, history and scope.

#TODO: Legge til at `--only-bar-color`  og `--sort-descending` er standard out of the box xp.
#TODO: Legge til while loop og dropdown hvor du kan cycle gjennom flag option
#IDEA: Legge til at du kan skifte scole. F.eks at vi vil se results fra `dirstat-total-file-dir-sizes` og `dirtat` <== basicly samme som `tree` ==> idé, vi gjør `tree` navigerbar?
#IDEA: Legge til support for locale formattering av tall osv

## Usage

### Options:
  `-h`, `--help`           Display this help message\
  `--no-color`          Disable colorized output\
  `--toggle-ascii`      Use ASCII instead of Unicode blocks\
  `--only-bar-color`    Color only bars, not text\
  `--exclude=pattern`   Exclude paths containing pattern

### Sorting Options:
  `--sort-descending`   Sort by count descending (default)\
  `--sort-ascending`    Sort by count ascending\
  `--sort-alpha-asc`    Sort alphabetically A-Z\
  `--sort-alpha-desc`   Sort alphabetically Z-A\
  `--sort-num-asc`      Sort numerically ascending\
  `--sort-num-desc`     Sort numerically descending\
  `--sort-natural-asc`  Sort with natural ordering ascending\
  `--sort-natural-desc` Sort with natural ordering descending\
  `--case-sensitive`    Make alphabetical sorting case-sensitive

Examples: 
```dirstat-project-size  --sort-descending --only-bar-color```
![image](https://github.com/user-attachments/assets/288a7dff-27b2-4292-97c8-6d172c76ee7d)

Result:

![image](https://github.com/user-attachments/assets/11652dba-ebd3-4fbd-a347-a7cb266baf9b)

