# make sure we have a datafile
filename = "`echo $GP_DATAFILE`"
if (strlen(filename) == 0) print "Error: No datafile name provided in ENV{GP_DATAFILE}"
if (strlen(filename) == 0) quit

# set view angle, rotation
# set view 60,30 # default view
#set view 35,30
set xlabel "Items"
set ylabel "Band"
set zlabel "Relevance"
set title "Relevance for items in bands"
set ytics 5

splot \
	filename title "Relevance" \
		with points lc rgb '#0060ad', \
	'' title "" lc rgb '#888888' lt 2 with lines
	
