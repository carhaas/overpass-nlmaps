The file cfg_nlmaps defines all rules needed to determine if a NLmaps MRL is valid or not. The CFG assumes that the different elements in the MRL are seperated by white space.

For the key and value positions (generally the first and second argument of keyval() respectively) the following assumptions are made:
- Any string is allowed in the value position but it is assumed that these value positions are replaced with ' valvariable ' (including the quotes) prior to consulting the CFG.
	This is done to ensure that truly any string can be in a value positions; rather than giving a long list of allowed characters that would most likely never be exhaustive
	The replacement can be easily done with the knowledge that value positions always equal the second argument in keyval() in a valid NLmaps MRL
	Special care needs to be taken though if and() or or() are involved
- For key positions, only common key tags from the OSM community are allowed: the list supplied in the cfg closely follows the list given here: http://wiki.openstreetmap.org/wiki/Map_Features
	The list can of course easily be extended or left similarly open as the value position by replacing all key positions with ' keyvariable '
	Key positions in a correct NLmaps MRL are always the first argument in keyval() or as an argument of findkey()
	Special care needs to be taken though if and() or or() are involved
