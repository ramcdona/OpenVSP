from bokeh.models.widgets import DataTable, DateFormatter, TableColumn
from bokeh.models import ColumnDataSource, DataTable
from pandas import DataFrame
from bokeh.plotting import show
from bokeh.core.enums import Align, SizingPolicy
import Constants as const
from bokeh.io import export_png

#table_headers = ["Case #","Analysis","VLM","alpha (°)","beta (°)","M","Wake Iterations"]
#table_data = [[1],["Sweep"],["VLM"],["-20.0 to 20.0, npts: 8"],[0.0],[.1],[3]]

#Creates a table from list of strings (header) that are the collum lables
#and list of lists that are the corrosponding data for each label(data)
def make_table(header,data):

    col_src = {}
    for i in range(len(header)):
        col_src[header[i]] = data[i]

    Columns = [TableColumn(field=name, title=name) for name in header] # bokeh columns
    data_table = DataTable(columns=Columns, source = ColumnDataSource(data = col_src)) # bokeh table
    data_table.index_position = None
    data_table.autosize_mode = 'fit_columns'
    data_table.height = 30+25 * len(data[0])
    data_table.width = const.bokehwidth
    return data_table

if __name__ == "__main__":
    header = const.STUDY_SETUP_TABLE_HEADER.copy() + ["Preconditioner","Mach Correction","Exe Time (sec)"]
    data_base = [["Default","1","2","3"],["Single Point"]*4,["VLM"]*4,["1.0"]*4,[0.0]*4,[const.m_MachVec[0]]*4]

    #Wake Iter = 1 Setup Table
    data = data_base + [[1]*4,["Matrix","Jacobi","SSOR","Matrix"],["Off"]*3+["On"],[0 for t in range(4)]]
    table = make_table(header,data)
    show(table)
