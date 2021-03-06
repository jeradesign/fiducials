// Copyright (c) 2013 by Wayne C. Gramlich.  All rights reserved

#include <assert.h>

#include "Arc.h"
#include "Double.h"
#include "File.h"
#include "Map.h"
#include "SVG.h"
#include "Tag.h"
#include "Unsigned.h"

// *Arc* routines:

/// @brief Return the sort order of *arc1* vs. *arc2*.
/// @param arc1 is the first *Arc* object.
/// @param arc2 is the first *Arc* object.
/// @returns -1, 0, or 1 depending upon sort order.
///
/// *Arc__compare*() will return -1 if *arc1* sorts before *arc2*,
/// 0 if they are equal, and 1 if *arc1* sorts after *arc2*.

Integer Arc__compare(Arc arc1, Arc arc2) {
    Integer result = Tag__compare(arc1->from_tag, arc2->from_tag);
    if (result == 0) {
	result = Tag__compare(arc1->to_tag, arc2->to_tag);
    }
    return result;
}

/// @brief Create and return a new *Arc* object.
/// @param origin *Tag*.
/// @param target *Tag*.
/// @param distance between *origin* and *target*.
/// @param target_angle is angle from *origin* center to *target* center.
/// @param target_twist is twist from *origin* to *target*. 
/// @returns new *Arc* object.
///
/// *Arc__create*() will create and return arc a new *Arc* object that
/// contains *from*, *to*, *distance*, *angle*, *twist* and *goodness*.
/// Both *from* and *to* are *Tag* objects.  *distance* is the
/// distance between the fiducial centers measured in the consistent
/// set of distance units (e.g. mm, cm, meter, etc.)  *angle* is the
/// the angle measued in radians from the *from* "X Axis" (see Tag.h)
/// line that connects *to* to *frm*.  *twist* is the angle measures
/// amount that *to* is twisted relative to *from*.

Arc Arc__create(Tag from_tag, Double from_twist,
  Double distance, Tag to_tag, Double to_twist, Double goodness) {
    // Make sure *from* id is less that *to* id:
    if (from_tag->id > to_tag->id) {
        // Compute the conjugate *Arc* (see Arc.h):
	Tag temporary_tag = from_tag;
	from_tag = to_tag;
	to_tag = temporary_tag;

	Double temporary_twist = from_twist;
	from_twist = to_twist;
	to_twist = temporary_twist;
    }

    // Create and load *arc*:
    Arc arc = Arc__new();
    arc->distance = distance;
    arc->from_tag = from_tag;
    arc->from_twist = from_twist;
    arc->goodness = goodness;
    arc->to_tag = to_tag;
    arc->to_twist = to_twist;

    // Append *arc* to *from*, *to*, and *map*:
    Tag__arc_append(from_tag, arc);
    Tag__arc_append(to_tag, arc);
    Map__arc_append(from_tag->map, arc);

    return arc;
}

/// @brief Return the distance sort order of *arc1* vs. *arc2*.
/// @param arc1 is the first *Arc* object.
/// @param arc2 is the second *Arc* object.
/// @returns -1, 0, or 1 depending upon distance sort order.
///
/// *Arc__distance_compare*() will return -1 if the *arc1* distance is larger
/// than the *arc2* distance, 0 if they are equal, and 1 otherwize.

Integer Arc__distance_compare(Arc arc1, Arc arc2) {
    Integer result = -Double__compare(arc1->distance, arc2->distance);
    if (result == 0) {
	Unsigned arc1_lowest_hop_count =
	  Unsigned__minimum(arc1->from_tag->hop_count, arc1->to_tag->hop_count);
	Unsigned arc2_lowest_hop_count =
	  Unsigned__minimum(arc2->from_tag->hop_count, arc2->to_tag->hop_count);
	result =
	  -Unsigned__compare(arc1_lowest_hop_count, arc2_lowest_hop_count);
    }
    return result;
}

/// @brief Return true if *arc1* equals *arc2*.
/// @param arc1 is the first *Arc* object.
/// @param arc2 is the first *Arc* object.
/// @returns true if they are equal.
///
/// *Arc__equal*() will return true if *arc1* is equal to *arc2*.

Logical Arc__equal(Arc arc1, Arc arc2) {
    return (Logical)(Arc__compare(arc1, arc2) == 0);
}

/// @brief Returns a hash value for *arc*.
/// @param arc to hash.
/// @returns hash value for *arc*.
///
/// *Arc__hash*() will return a hash value for *arc*.

Unsigned Arc__hash(Arc arc) {
    return Tag__hash(arc->from_tag) + Tag__hash(arc->to_tag);
}

/// @brief Returns a new *Arc* object.
/// @returns new *Arc* object.
///
/// *Arc__new*() will return a new *Arc*.

Arc Arc__new(void) {
    Arc arc = Memory__new(Arc);
    arc->distance = 0.0;
    arc->from_tag = (Tag)0;
    arc->from_twist = 0.0;
    arc->goodness = 123456789.0;
    arc->in_tree = (Logical)0;
    arc->to_tag = (Tag)0;
    arc->to_twist = 0.0;
    arc->visit = 0;
    return arc;
}

/// @brief Read in an XML <Arc.../> tag from *in_file*.
/// @param in_file is the file to read from.
/// @param map is contains the Tag associations.
/// @returns new *Arc* object.
///
/// *Arc__read*() will read in a <Arc.../> tag from *in_file*
/// and return the resulting *Arc* object.  *Tag* objects all looked
/// up using *map*.

Arc Arc__read(File in_file, Map map) {
    // Read <Arc ... /> tag:
    File__tag_match(in_file, "Arc");
    Unsigned from_tag_id =
      (Unsigned)File__integer_attribute_read(in_file, "From_Tag_Id");
    Double from_twist = File__float_attribute_read(in_file, "From_Twist");
    Double distance = File__float_attribute_read(in_file, "Distance");
    Unsigned to_tag_id =
       (Unsigned)File__integer_attribute_read(in_file, "To_Tag_Id");
    Double to_twist = File__float_attribute_read(in_file, "To_Twist");
    Double goodness = File__float_attribute_read(in_file, "Goodness");
    Logical in_tree = (Logical)File__integer_attribute_read(in_file, "In_Tree");
    File__string_match(in_file, "/>\n");

    // Convert from degrees to radians:
    Double pi = (Double)3.14159265358979323846264;
    Double radians_to_degrees =  pi / 180.0;
    from_twist *= radians_to_degrees;
    to_twist *= radians_to_degrees;

    // Create and load *arc*:
    Tag from_tag = Map__tag_lookup(map, from_tag_id);
    Tag to_tag = Map__tag_lookup(map, to_tag_id);
    Arc arc =
      Arc__create(from_tag, from_twist, distance, to_tag, to_twist, goodness);
    arc->in_tree = in_tree;
    return arc;
}

void Arc__svg_write(Arc arc, SVG svg) {
    Tag from_tag = arc->from_tag;
    Tag to_tag = arc->to_tag;
    String color = "green";
    if (arc->in_tree) {
	color = "red";
    }
    SVG__line(svg, from_tag->x, from_tag->y, to_tag->x, to_tag->y, color);
}

/// @brief Updates the contenst of *arc*.
/// @param arc to update.
/// @param distance to load into *arc*.
/// @param angle to load into *arc*.
/// @param twist to load into *arc*.
/// @param goodness to load into *arc*.
///
/// *Arc__update*() will load *distance*, *angle*, *twist*, and *goodness*
/// into *arc*.

void Arc__update(Arc arc,
 Double from_twist, Double distance, Double to_twist, Double goodness) {
    // Create and load *arc*:
    assert (arc->from_tag->id < arc->to_tag->id);
    arc->from_twist = from_twist;
    arc->distance = distance;
    arc->goodness = goodness;
    arc->to_twist = to_twist;
}

/// @brief Write *arc* out to *out_file* in XML format.
/// @param arc to be written out.
/// @param out_file to write ot.
///
/// *Arc__write*() will write *arc* out to *out_file* as an
/// <Arc .../> tag.

void Arc__write(Arc arc, File out_file) {
    // We need to convert from radians to degrees:
    Double pi = (Double)3.14159265358979323846264;
    Double radians_to_degrees = 180.0 / pi;
    Double from_twist_degrees = arc->from_twist * radians_to_degrees;
    Double to_twist_degrees = arc->to_twist * radians_to_degrees;

    // Output <Arc ... /> tag to *out_file*:
    File__format(out_file, " <Arc");
    File__format(out_file, " From_Tag_Id=\"%d\"", arc->from_tag->id);
    File__format(out_file, " From_Twist=\"%f\"", from_twist_degrees);
    File__format(out_file, " Distance=\"%f\"", arc->distance);
    File__format(out_file, " To_Tag_Id=\"%d\"", arc->to_tag->id);
    File__format(out_file, " To_Twist=\"%f\"", to_twist_degrees);
    File__format(out_file, " Goodness=\"%f\"", arc->goodness);
    File__format(out_file, " In_Tree=\"%d\"", arc->in_tree);
    File__format(out_file, "/>\n");
}

