/*
 * 3df2obj.c
 * Copyright 2020 Filip Aláč
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 * This tool convert .3df file from 1997 DSI game Moto Racer to .obj file
 *
 * File Structure:
 *
 * Type: BINARY
 * Byte-order: LITTLE ENDIAN
 *
 *
 * Header(4B):
 * Is 4 byte long. Third byte is saying  how many vertices is in the file.
 *
 * Vertex(12B) list:
 * Each vertex consist of 3 floating point numbers for X,Y,Z values.
 *
 * one face Header(2B):
 * Says if first face is quad(04) or triangle(03).
 *
 * Face(12B if quad, 10B if tri) list:
 * Is like this if face is quad:
 *  	uint16 number of vertices in current face(3 or 4)
 *  	uint16 index of 1st vertex
 *  	uint16 index of 2nd vertex
 *  	uint16 index of 3rd vertex
 *  	uint16 index of 4th vertex
 *  	uint16 number of vertices in next face(3,4 or 0 if it is last face)
 *  And if face is triangle, 4th vertex is ommitted, and total size of face is 10B
 *
 *  It seems to use uint16 for indexes but there probably is not
 *  any .3df file with more than 255 vertices.
 */


#include <stdio.h>
#include <inttypes.h>


struct vertex_data {
	float x;
	float y;
	float z;
};


struct tri_face {
	uint16_t current_size;
	uint16_t vertex1;
	uint16_t vertex2;
	uint16_t vertex3;
	uint16_t next_size;
};


struct quad_face {
	uint16_t current_size;
	uint16_t vertex1;
	uint16_t vertex2;
	uint16_t vertex3;
	uint16_t vertex4;
	uint16_t next_size;
};


union face_union {
	struct tri_face tri;
	struct quad_face quad;
};


uint16_t print_face(union face_union f, uint16_t size, FILE *file) {
	uint16_t next_size;

	if (size == 4) {
		fprintf(file, "f %d %d %d %d\n", f.quad.vertex1 + 1,
			       		       	 f.quad.vertex2 + 1,
						 f.quad.vertex3 + 1,
						 f.quad.vertex4 + 1);
		next_size = f.quad.next_size;

	} else {
		fprintf(file, "f %d %d %d \n", f.tri.vertex1 + 1,
			       		       f.tri.vertex2 + 1,
					       f.tri.vertex3 + 1);
		next_size = f.tri.next_size;
	}

	return next_size;
}


int main(int argc, char **argv)
{
	FILE *file_in = fopen(argv[1], "rb");
	FILE *file_out = fopen(argv[2], "w+");

	uint8_t vertex_count = 0;

	// Get vertices count from third byte
	for (int i = 0; i < 4; i++) {
		int tmp = 0;
		fread(&tmp, 1, 1,  file_in);
		if (i == 2)
			vertex_count = tmp;
	}

	// print vertices to file
	for (int i = 0; i < vertex_count; i++) {
		struct  vertex_data v;

		fread(&v, 1, sizeof(struct vertex_data),  file_in);
		fprintf(file_out, "v %f %f %f\n", v.x, v.y, v.z);
	}

	// Get size of first face from header
	uint16_t next_size = 0;
	fread(&next_size, 1, sizeof(uint16_t),  file_in);

	// Repeat until next_size from last face is 0, that means EOF
	while(next_size != 0) {
		union face_union face;

		int size = next_size == 4 ? sizeof(struct quad_face) :
			      		      sizeof(struct tri_face);

		if (fread(&face, 1, size, file_in) != size) {
			printf("Error reading file! exiting.\n");
			return 1;
		}

		next_size = print_face(face, next_size, file_out);
	}

	fclose(file_in);
	fclose(file_out);
}
