cmd_/home/bobous/cmpt433/work/as4/Module.symvers := sed 's/ko$$/o/' /home/bobous/cmpt433/work/as4/modules.order | scripts/mod/modpost -m    -o /home/bobous/cmpt433/work/as4/Module.symvers -e -i Module.symvers   -T -