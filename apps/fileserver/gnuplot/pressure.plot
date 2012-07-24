# reset
# 
# set border linewidth 1
# set pointintervalbox 30

# output to pdf
set terminal pdf

# allow dashed lines
set termoption dash

#set style line 1 linecolor rgb '#0060ad' linewidth 2 pointinterval -1

# make sure we have a datafile
filename = "`echo $GP_DATAFILE`"
if (strlen(filename) == 0) print "Error: No datafile name provided in ENV{GP_DATAFILE}"
if (strlen(filename) == 0) quit

# increase range on y1
set yrange [-0.1:1.1]

# do not put y1 ticks on y2
set ytics nomirror 

set y2tics
set xlabel "Iteration"
set ylabel "Pressure"
set y2label "Overflow in kB"
set title "Convergence of cache pressure"

#set grid
#unset border 
#set boxwidth 0.3

show style line

plot \
	filename using (($5 / 1024)-($6 / 1024)) \
		title "overflow" with points \
		lc rgb '#ad6000' pt 4 lw 2 axes x1y2, \
	'' using (($5 / 1024)-($6 / 1024)) \
		title "" with lines \
		lc rgb '#888888' lt 2 axes x1y2, \
	0 with lines title "" lt 1 lc rgb '#888888' axes x1y2, \
	'' using 1:2:3:4 \
		title "pressure" with yerrorbars \
		lc rgb '#0060ad' lw 2 lt 1 axes x1y1, \
	'' using 2 \
		title "" with lines \
		lc rgb '#888888' lt 2 axes x1y1


# set logscale
