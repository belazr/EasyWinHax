import math
import re
from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def calculate_min_dist_to_edge(data_frame_edges_only, x,y):

    distances = []

    for _, row in data_frame_edges_only.iterrows():

        x_edge = row.x
        y_edge = row.y

        distance = math.dist([x,y], [x_edge,y_edge])

        distances.append(distance)

        min_distance = min(distances)

    return min_distance


def plot_and_save(file_name, data_mask, show):

    pixel_plot = plt.figure()
    plt.title(file_name)
    pixel_plot = plt.imshow(data_mask)
    plt.colorbar(pixel_plot)

    plt.savefig(f'{file_name}.png')
    if show:
        plt.show()


def write_character_substring(name, data_frame):

    substring = f"\t\t\t\tconstexpr Vector2{name}[]{{ \n"

    for _, rows in data_frame.iterrows():

        line = "\t\t\t\t\t{ { " + str(rows.x) + ", " + str(rows.y) + " }, " + str(round(rows.edge)) + " }," + "\n"

        substring += line

    substring += "\t\t\t\t };\n"

    return substring

#%%
DIVIDER = "constexpr Vector2"
FILE_NAMES =  ["dxLargeFont.cpp", "dxMediumFont.cpp", "dxTinyFont.cpp"]

BORDER_VALUE = 1
INSIDE_VALUE = 2

for file_name in FILE_NAMES:

    txt = Path(file_name).read_text()

    split_end = txt.split("constexpr Font")
    end_of_file = "\t\t\tconstexpr Font" + split_end[1]
    split_characters = split_end[0].split(DIVIDER)
    beginning_of_file = split_characters[0] + "\n"

    txt_new = beginning_of_file

#%%
    for character_text in split_characters[1:]:

        name = character_text.split("[]")[0]

        print(name)

        character_only_numbers = re.sub(r'[^0-9,]', '', character_text)

        # Split data into x and y
        X = []
        Y = []

        entries = character_only_numbers.split(",")

        for index, entry in enumerate(entries):

            if len(entry) > 0:

                if (index % 2) == 0:

                    X.append(int(entry))

                else:

                    Y.append(int(entry))

        # Organize in data frame
        data_frame = pd.concat([pd.Series(X), pd.Series(Y)], axis=1)
        data_frame = data_frame.rename(columns={0: "x", 1: "y"})
        data_frame["edge"] = False

        # Create map for plotting for QA
        character_map = np.zeros((max(data_frame['y']) + 5, max(data_frame['x']) + 5))

        for index, row in data_frame.iterrows():

            x = row.x
            y = row.y

            character_map[y,x] = BORDER_VALUE

        data_mask = character_map.copy()

        # Mark each pixel within the borders with 2
        for y in range(character_map.shape[0]-1):

            for x in range(character_map.shape[1]-1):

                neighbour0 = character_map[y,x-1]
                neighbour1 = character_map[y,x+1]
                neighbour2 = character_map[y-1,x]
                neighbour3 = character_map[y+1,x]
                neighbour4 = character_map[y-1,x-1]
                neighbour5 = character_map[y+1,x+1]
                neighbour6 = character_map[y-1,x-1]
                neighbour7 = character_map[y+1,x+1]

                sum_neighbours = neighbour0 + neighbour1 + neighbour2 + neighbour3 + neighbour4 + neighbour5 + neighbour6 + neighbour7

                if sum_neighbours == 8:

                    data_mask[y,x] = INSIDE_VALUE


        for y in range(data_mask.shape[0] - 1):

            for x in range(data_mask.shape[1] - 1):

                if data_mask[y,x] == BORDER_VALUE:

                    data_frame.loc[(data_frame['x'] == x) & (data_frame['y'] == y), 'edge'] = True


        data_mask[data_mask == 0] = 5 # Overwrite zeros for plotting
        data_frame_edges_only = data_frame[data_frame['edge'] == True]


        #Calculate minimum distance for each pixel within the character
        for index, row in data_frame.iterrows():

            x = row.x
            y = row.y

            min_dist = calculate_min_dist_to_edge(data_frame_edges_only, x, y)

            data_frame.loc[(data_frame['x'] == x) & (data_frame['y'] == y), 'edge'] = min_dist

            data_mask[y,x] = min_dist


        plot_and_save("images/" + file_name + "/" + name, data_mask, False)

        substring = write_character_substring(name, data_frame)

        txt_new += substring

    txt_new += end_of_file

    # Save to file
    with open("shorty_" + file_name, mode="w") as f:
        f.write(txt_new)