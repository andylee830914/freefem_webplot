var draw = SVG().addTo('#new_plot');

//output btn for 2d SVG
$('#2dsvg').click(function (e) {
    preview = 0;
    mydraw2d();
    var text = draw.svg();
    var file = new Blob([text], { type: 'image/svg+xml' });
    var url = URL.createObjectURL(file);
    // var w = window.open(url, 'test.svg');
    this.download = 'output-' + now_file + '.svg';
    this.href = url;
    preview = 1;
});

//output btn for 2D PNG
$('#2dpng').click(function () {
    preview = 0;
    mydraw2d_canvas();
    var w1 = window.open("", 'test.png'); // to prevent browser block popup window
    var canvas = document.getElementById("canvas_2d");
    var imgURI = canvas.toDataURL('image/png')
    var blob = dataURItoBlob(imgURI)
    var url = URL.createObjectURL(blob);
    w1.location.href = url;
    preview = 1;

});

// 2D Draw
function mydraw2d() {

    var max_x = basic_data.bounds[1][0] - basic_data.bounds[0][0];
    var max_y = basic_data.bounds[1][1] - basic_data.bounds[0][1];
    var max = Math.max(max_x, max_y);
    // var max = max_y;
    const sc = 500 / max;
    const w = 1 * sc;
    const h = -1 * sc;
    try {
        draw.clear()
    } catch (error) {
        console.log(error);
    }

    $("#new_plot > svg").removeAttr('height');


    if ($("#level").is(':checked') && $("#colorbar").is(':checked')) {
        draw_viewbox_w_scale = 0.02 * max_y * 10;
        draw_colorbox = true;
    } else {
        draw_viewbox_w_scale = 0;
        draw_colorbox = false;

    }

    draw.viewbox({
        x: (-0.05 * max_x + basic_data.bounds[0][0]) * sc
        , y: (-0.05 * max_y - basic_data.bounds[1][1]) * sc
        , width: (1.1 * max_x + draw_viewbox_w_scale) * sc,
        height: (1.1 * max_y) * sc
    })

    if (preview == 1) {
        if (!$("#svg").is(':checked')) {
            $("#new_plot").children('svg').hide();
            $("#canvas_2d").show();
            mydraw2d_canvas();
            return;
        } else {
            $("#new_plot").children("svg").show();
            $("#canvas_2d").hide();

        }
    }

    var gradient = draw.gradient('linear', function (add) {
        add.stop(0, cprofile[0])
        add.stop(0.25, cprofile[1])
        add.stop(0.50, cprofile[2])
        add.stop(0.75, cprofile[3])
        add.stop(1, cprofile[4])

    })


    var color = [
        new SVG.Color(cprofile[0]).to(cprofile[1]),
        new SVG.Color(cprofile[1]).to(cprofile[2]),
        new SVG.Color(cprofile[2]).to(cprofile[3]),
        new SVG.Color(cprofile[3]).to(cprofile[4])
    ]


    if ($("#mesh").is(':checked')) {
        mesh_data.forEach((e, i) => {
            var polyline = draw.polygon([[e[0].x * w, e[0].y * h], [e[1].x * w, e[1].y * h], [e[2].x * w, e[2].y * h]])
            polyline.fill('none').stroke({ color: '#000000', width: 1 / 500 * sc })
            if ($("#tri_index").is(':checked')) {
                var text = draw.plain(i).attr('x', (e[0].x + e[1].x + e[2].x) / 3 * w).attr('y', (e[0].y + e[1].y + e[2].y) / 3 * h)
                    .font({
                        size: 0.02 * sc + "px",
                        anchor: 'middle'
                    })
            }
        });

        if ($("#ver_index").is(':checked') && typeof vertex_data !== 'undefined') {
            vertex_data.forEach((e, i) => {
                var circle = draw.circle(1 / 100 * sc).attr('cx', e.x * w).attr('cy', e.y * h);
                var text = draw.plain(i).attr('x', (e.x - 0.015) * w).attr('y', (e.y + 0.01) * h)
                    .font({
                        size: 0.02 * sc + "px",
                        anchor: 'middle',
                        fill: '#0000FF'
                    })
            })

        }
    } else {
        edge_data.forEach((e, i) => {
            var line = draw.line(e.vertices[0].x * w, e.vertices[0].y * h, e.vertices[1].x * w, e.vertices[1].y * h)
                .stroke({ color: '#000000', width: 1 / 500 * sc });
        })
    }




    if ($("#edg_label").is(':checked') && typeof edge_data !== 'undefined') {
        edge_data.forEach((e, i) => {
            var line = draw.line(e.vertices[0].x * w, e.vertices[0].y * h, e.vertices[1].x * w, e.vertices[1].y * h)
                .stroke({ width: 1 / 500 * sc }).stroke({ color: '#f06' });
            var text = draw.plain(e.label).attr('x', (e.vertices[0].x + e.vertices[1].x) / 2 * w - 0.015 * sc).attr('y', (e.vertices[0].y + e.vertices[1].y) / 2 * h)
                .font({
                    size: 0.02 * sc + "px",
                    anchor: 'middle',
                    fill: '#f06'
                })
        })

    }

    if ($("#level").is(':checked') && typeof mesh_data !== 'undefined') {
        if (draw_colorbox) {
            var rect = draw.rect(max_y * sc, 0.02 * max_y * sc).fill(gradient)
                .attr('x', (basic_data.bounds[1][0] + draw_viewbox_w_scale - 1.15 * max_y) * w)
                .attr('y', (0.02 * max_y + basic_data.bounds[1][1]) * h)
                .rotate(270, (basic_data.bounds[1][0] + draw_viewbox_w_scale - 0.13 * max_y) * w, (0.02 * max_y + basic_data.bounds[1][1]) * h)
        }


        var nlevel = Number($("#nbiso").val()) - 1;
        var vlevel = [];
        var eps = 1e-10;
        var dl = (minmax_data[1].u - minmax_data[0].u) / nlevel;


        for (let i = 0; i <= nlevel; i++) {
            let tempval = minmax_data[0].u + i * dl;
            if (Math.abs(tempval) < eps) {
                tempval = 0;
            }
            vlevel.push(tempval);
        }

        // if (vlevel[0] >= vlevel[1]) {
        //     vlevel[0] = vlevel[0] - eps;
        //     vlevel[nlevel] = vlevel[nlevel] + eps;
        // } else {
        //     vlevel[0] = vlevel[0] + eps;
        //     vlevel[nlevel] = vlevel[nlevel] - eps;
        // }
        var drew_equal = [];
        for (let vi = vlevel.length - 1; vi >= 0; vi--) {
            const ve = vlevel[vi];
            var text_space = max_y / nlevel;
            var cr = 4 * vi / nlevel;
            var ci = Math.floor(cr);
            if (ci == 4) {
                ci = ci - 1;
            }
            if ((vi == 0 || vi == vlevel.length - 1 || vi % 2 == 0) && draw_colorbox) {
                var text = draw.plain(vlevel[vi].toExponential(3)).attr('x', (basic_data.bounds[1][0] + draw_viewbox_w_scale - 0.03 * max_y) * w).attr('y', (basic_data.bounds[1][1] - text_space * (vlevel.length - vi - 1)) * h)
                    .font({
                        size: 0.03 * sc + "px",
                        anchor: 'middle',
                        fill: color[ci].at(cr - ci).toHex()
                    });
            }

            for (let i = 0; i < mesh_data.length; i++) {
                const e = mesh_data[i];
                var level_line = [];
                if ((ve >= e[0].u && ve <= e[1].u) || (ve <= e[0].u && ve >= e[1].u)) {
                    //at 1st edge of the element
                    var rate = (ve - e[0].u) / (e[1].u - e[0].u) || 0;
                    level_line.push([(rate * e[1].x + (1 - rate) * e[0].x) * w, (rate * e[1].y + (1 - rate) * e[0].y) * h])
                }
                if ((ve >= e[1].u && ve <= e[2].u) || (ve <= e[1].u && ve >= e[2].u)) {
                    //at 2nd edge of the element
                    var rate = (ve - e[1].u) / (e[2].u - e[1].u) || 0;
                    level_line.push([(rate * e[2].x + (1 - rate) * e[1].x) * w, (rate * e[2].y + (1 - rate) * e[1].y) * h])
                }
                if ((ve >= e[2].u && ve <= e[0].u) || (ve <= e[2].u && ve >= e[0].u)) {
                    //at 3rd edge of the element
                    var rate = (ve - e[2].u) / (e[0].u - e[2].u) || 0;
                    level_line.push([(rate * e[0].x + (1 - rate) * e[2].x) * w, (rate * e[0].y + (1 - rate) * e[2].y) * h])
                }
                if (level_line.length > 2) {
                    if ((ve >= e[0].u && ve <= e[1].u) || (ve <= e[0].u && ve >= e[1].u)) {
                        //at 1st edge of the element
                        var rate = (ve - e[0].u) / (e[1].u - e[0].u) || 0;
                        level_line.push([(rate * e[1].x + (1 - rate) * e[0].x) * w, (rate * e[1].y + (1 - rate) * e[0].y) * h])
                    }
                }
                if ((e[0].u == e[1].u) && (e[0].u == e[2].u) && ((minmax_data[1].u - minmax_data[0].u) > 0) && !drew_equal.includes(i)) {
                    var cr = 4 * (e[0].u - minmax_data[0].u) / (minmax_data[1].u - minmax_data[0].u);
                    var ci = Math.floor(cr);
                    if (ci == 4) {
                        ci = ci - 1;
                    }
                    var polyline = draw.polygon([[e[0].x * w, e[0].y * h], [e[1].x * w, e[1].y * h], [e[2].x * w, e[2].y * h]])
                    polyline.fill('none').stroke({ color: color[ci].at(cr - ci).toHex(), width: 1 / 500 * sc })
                    drew_equal.push(i);
                } else {
                    draw.polyline(level_line).fill('none').stroke({ width: 1 / 250 * sc, color: color[ci].at(cr - ci).toHex() });
                }

            }
        }
    }



}


function mydraw2d_canvas() {
    var max_x = basic_data.bounds[1][0] - basic_data.bounds[0][0];
    var max_y = basic_data.bounds[1][1] - basic_data.bounds[0][1];
    // var max = Math.max(max_x, max_y);
    var max = max_y;
    var c = {
        x: -0.05 * max_x + basic_data.bounds[0][0],
        y: -0.05 * max_y - basic_data.bounds[1][1],
    }

    if ($("#canvas_2d").length == 0) {
        var canvas = document.createElement('canvas');
        canvas.id = "canvas_2d";
        // canvas.style.border = "1px solid";
        if (preview == 0) {
            if ($("#svg").is(':checked')) {
                canvas.style.display = "none";
            }
        }
        var body = document.getElementById("new_plot");
        body.appendChild(canvas);


    }
    var box = draw.viewbox();
    $("#canvas_2d").attr('width', 1024 * box.width / box.height).attr('height', '1024');

    const cw = 1024 / 1.1;
    const sc = cw / max;
    const w = 1 * sc;
    const h = -1 * sc;

    var canvas = document.getElementById('canvas_2d');
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    var color = [
        new SVG.Color(cprofile[0]).to(cprofile[1]),
        new SVG.Color(cprofile[1]).to(cprofile[2]),
        new SVG.Color(cprofile[2]).to(cprofile[3]),
        new SVG.Color(cprofile[3]).to(cprofile[4])
    ]

    if ($("#mesh").is(':checked')) {
        mesh_data.forEach((e, i) => {
            ctx.beginPath();
            ctx.moveTo((e[0].x - c.x) * w, (e[0].y + c.y) * h);
            ctx.lineTo((e[1].x - c.x) * w, (e[1].y + c.y) * h);
            ctx.lineTo((e[2].x - c.x) * w, (e[2].y + c.y) * h);
            ctx.lineTo((e[0].x - c.x) * w, (e[0].y + c.y) * h);
            ctx.strokeStyle = "#000000";
            ctx.lineWidth = 1 / 500 * sc;
            ctx.closePath();


            if ($("#tri_index").is(':checked')) {
                ctx.font = 0.02 * sc + "px  sans-serif";
                ctx.textAlign = "center";
                ctx.fillText(i, (e[0].x + e[1].x + e[2].x - 3 * c.x) / 3 * w, (e[0].y + e[1].y + e[2].y + 3 * c.y) / 3 * h);
            }
            ctx.stroke();

        });

        if ($("#ver_index").is(':checked') && typeof vertex_data !== 'undefined') {
            vertex_data.forEach((e, i) => {
                ctx.beginPath();
                ctx.arc((e.x - c.x) * w, (e.y + c.y) * h, 1 / 250 * sc, 0, Math.PI * 2);
                ctx.fillStyle = '#000000'
                ctx.fill();
                ctx.font = 0.02 * sc + "px  sans-serif";
                ctx.textAlign = "center";
                ctx.fillStyle = '#0000FF';
                ctx.fillText(i, (e.x - 0.015 - c.x) * w, (e.y + 0.01 + c.y) * h);
                ctx.stroke();

            })

        }
    } else {
        // console.log(edge_data);
        edge_data.forEach((e, i) => {
            ctx.beginPath();
            ctx.moveTo((e.vertices[0].x - c.x) * w, (e.vertices[0].y + c.y) * h);
            ctx.lineTo((e.vertices[1].x - c.x) * w, (e.vertices[1].y + c.y) * h);
            ctx.strokeStyle = "#000000";
            ctx.lineWidth = 1 / 500 * sc;
            ctx.closePath();
            ctx.stroke();
        })
    }

    if ($("#edg_label").is(':checked') && typeof edge_data !== 'undefined') {
        edge_data.forEach((e, i) => {
            ctx.beginPath();
            ctx.moveTo((e.vertices[0].x - c.x) * w, (e.vertices[0].y + c.y) * h);
            ctx.lineTo((e.vertices[1].x - c.x) * w, (e.vertices[1].y + c.y) * h);
            ctx.strokeStyle = "#f06";
            ctx.lineWidth = 1 / 500 * sc;
            ctx.closePath();
            ctx.font = 0.02 * sc + "px  sans-serif";
            ctx.textAlign = "center";
            ctx.fillStyle = '#f06'
            ctx.fillText(e.label, (e.vertices[0].x + e.vertices[1].x - 2 * c.x) / 2 * w - 0.015 * sc, (e.vertices[0].y + e.vertices[1].y + 2 * c.y) / 2 * h);
            ctx.stroke();

        })

    }

    if ($("#level").is(':checked') && typeof mesh_data !== 'undefined') {
        if (draw_colorbox) {

            // Create gradient
            var grd = ctx.createLinearGradient(0, max_y * sc, 0, -0.05 * max_y * h);
            grd.addColorStop(0, cprofile[0]);
            grd.addColorStop(0.25, cprofile[1]);
            grd.addColorStop(0.50, cprofile[2]);
            grd.addColorStop(0.75, cprofile[3]);
            grd.addColorStop(1, cprofile[4]);

            // Fill with gradient
            ctx.fillStyle = grd;
            ctx.fillRect((basic_data.bounds[1][0] + draw_viewbox_w_scale - 0.13 * max_y - c.x) * w, -0.05 * max_y * h, 0.02 * max_y * sc, max_y * sc);
        }


        var nlevel = Number($("#nbiso").val()) - 1;
        var vlevel = [];
        var eps = 1e-10;
        var dl = (minmax_data[1].u - minmax_data[0].u) / nlevel;

        for (let i = 0; i <= nlevel; i++) {
            let tempval = minmax_data[0].u + i * dl;
            if (Math.abs(tempval) < eps) {
                tempval = 0;
            }
            vlevel.push(tempval);
        }
        // if (vlevel[0] > vlevel[1]) {
        //     vlevel[0] = vlevel[0] - eps;
        //     vlevel[nlevel] = vlevel[nlevel] + eps;
        // } else {
        //     vlevel[0] = vlevel[0] + eps;
        //     vlevel[nlevel] = vlevel[nlevel] - eps;
        // }
        var drew_equal = [];
        for (let vi = vlevel.length - 1; vi >= 0; vi--) {
            const ve = vlevel[vi];
            var text_space = max_y / nlevel;
            var cr = 4 * vi / nlevel;
            var ci = Math.floor(cr);
            if (ci == 4) {
                ci = ci - 1;
            }
            if ((vi == 0 || vi == vlevel.length - 1 || vi % 2 == 0) && draw_colorbox) {
                ctx.font = 0.03 * sc + "px sans-serif";
                ctx.textAlign = "center";
                ctx.fillStyle = color[ci].at(cr - ci).toHex()
                ctx.fillText(vlevel[vi].toExponential(3), (basic_data.bounds[1][0] + draw_viewbox_w_scale - 0.03 * max_y - c.x) * w, (basic_data.bounds[1][1] - text_space * (vlevel.length - vi - 1) + c.y) * h);
            }

            for (let i = 0; i < mesh_data.length; i++) {
                const e = mesh_data[i];
                var level_line = [];
                if ((ve >= e[0].u && ve <= e[1].u) || (ve <= e[0].u && ve >= e[1].u)) {
                    //at 1st edge of the element
                    var rate = (ve - e[0].u) / (e[1].u - e[0].u) || 0;
                    level_line.push([(rate * (e[1].x - c.x) + (1 - rate) * (e[0].x - c.x)) * w, (rate * (e[1].y + c.y) + (1 - rate) * (e[0].y + c.y)) * h])
                }
                if ((ve >= e[1].u && ve <= e[2].u) || (ve <= e[1].u && ve >= e[2].u)) {
                    //at 2nd edge of the element
                    var rate = (ve - e[1].u) / (e[2].u - e[1].u) || 0;
                    level_line.push([(rate * (e[2].x - c.x) + (1 - rate) * (e[1].x - c.x)) * w, (rate * (e[2].y + c.y) + (1 - rate) * (e[1].y + c.y)) * h])
                }
                if ((ve >= e[2].u && ve <= e[0].u) || (ve <= e[2].u && ve >= e[0].u)) {
                    //at 3rd edge of the element
                    var rate = (ve - e[2].u) / (e[0].u - e[2].u) || 0;
                    level_line.push([(rate * (e[0].x - c.x) + (1 - rate) * (e[2].x - c.x)) * w, (rate * (e[0].y + c.y) + (1 - rate) * (e[2].y + c.y)) * h])
                }
                if ((e[0].u == e[1].u) && (e[0].u == e[2].u) && ((minmax_data[1].u - minmax_data[0].u) > 0) && !drew_equal.includes(i)) {
                    ctx.beginPath();
                    ctx.moveTo((e[0].x - c.x) * w, (e[0].y + c.y) * h);
                    ctx.lineTo((e[1].x - c.x) * w, (e[1].y + c.y) * h);
                    ctx.lineTo((e[2].x - c.x) * w, (e[2].y + c.y) * h);
                    ctx.lineTo((e[0].x - c.x) * w, (e[0].y + c.y) * h);
                    var cr = 4 * (e[0].u - minmax_data[0].u) / (minmax_data[1].u - minmax_data[0].u);
                    var ci = Math.floor(cr);
                    if (ci == 4) {
                        ci = ci - 1;
                    }
                    ctx.strokeStyle = color[ci].at(cr - ci).toHex();
                    ctx.lineWidth = 1 / 500 * sc;
                    ctx.closePath();
                    ctx.stroke();
                    drew_equal.push(i);
                } else {
                    ctx.beginPath();
                    level_line.forEach((e, i) => {
                        if (i == 0) {
                            ctx.moveTo(level_line[i][0], level_line[i][1]);
                        } else {
                            ctx.lineTo(level_line[i][0], level_line[i][1]);
                        }
                    });
                    if (level_line.length > 2) {
                        ctx.lineTo(level_line[0][0], level_line[0][1]);
                    }

                    ctx.strokeStyle = color[ci].at(cr - ci).toHex();
                    ctx.lineWidth = 1 / 250 * sc;
                    ctx.stroke();
                }


            }

        }
    }
}
