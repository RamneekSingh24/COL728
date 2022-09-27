from anytree import Node
from anytree.exporter import DotExporter
import sys


l_no = 0
lines = data = sys.stdin.readlines()
lines = [l.rstrip() for l in lines]



def parseAST(parent=None):
    global l_no
    node_text = lines[l_no]
    l_no += 1
    if "---" not in node_text:
        # terminal
        node = Node(node_text.lstrip() + " #" + str(l_no), parent=parent)
        # print(node)
        return node
    else:
        node = Node(node_text.lstrip().rstrip("-") + " #" + str(l_no), parent=parent)

        while True:
                if lines[l_no] == node_text:
                    l_no += 1
                    # print(node)
                    if len(node.children) == 0:
                        _ = Node(f"empty #{l_no}", node)
                    return node
                else:
                    parseAST(parent=node)

try:
    tree = parseAST()
    DotExporter(tree).to_picture("viz.png")
except Exception as e:
    print(e)
    exit(0)
