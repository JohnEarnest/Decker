// MScale2X algorithm by Morgan McGuire and Mara Gagiu

// The use of reverse alpha is because when M-scale needs a tie breaker,
// the rules favor dark curves on light backgrounds (to make black outlines look good),
// but we also want to favor all curves against transparent backgrounds.
function luma (C) {
  const alpha = (C & 0xFF000000) >>> 24
  return (((C & 0x00FF0000) >> 16) + ((C & 0x0000FF00) >> 8) + (C & 0x000000FF) + 1) * (256 - alpha)
}

//    return B === A0 && B === A1;
// Optimized (faster bitwise is more important than early out)
function all_eq2 (B, A0, A1) {
  return ((B ^ A0) | (B ^ A1)) === 0
}

//    return B === A0 && B === A1 && B === A2;
// Optimized (faster bitwise is more important than early out)
function all_eq3 (B, A0, A1, A2) {
  return ((B ^ A0) | (B ^ A1) | (B ^ A2)) === 0
}

//    return B === A0 && B === A1 && B === A2 && B === A3;
// Optimized (faster bitwise is more important than early out)
function all_eq4 (B, A0, A1, A2, A3) {
  return ((B ^ A0) | (B ^ A1) | (B ^ A2) | (B ^ A3)) === 0
}

function any_eq3 (B, A0, A1, A2) {
  return B === A0 || B === A1 || B === A2
}

// Bitwise compiles to incorrect result and gives no speedup

function none_eq2 (B, A0, A1) {
  return (B !== A0) && (B !== A1)
  // return ((B ^ A0) | (B ^ A1)) === 0;
}

function none_eq4 (B, A0, A1, A2, A3) {
  return B !== A0 && B !== A1 && B !== A2 && B !== A3
  // return ((B ^ A0) | (B ^ A1) | (B ^ A2) | (B ^ A3)) === 0;
}

mmpx = function(srcBuffer, srcWidth, srcHeight, dstBuffer) {
  function src (x, y) {
    // Clamp to border
    return srcBuffer[Math.max(0, Math.min(x, srcWidth - 1)) +
                         Math.max(0, Math.min(y, srcHeight - 1)) * srcWidth] >>> 0
  };

  // If it was important to run this faster (e.g., as a full screen
  // filter) then it would make sense to remove the alpha cases and
  // to precompute horizontal and vertical binary gradients
  // (inequalities), since most of the tests are redundantly looking
  // at whether adjacent elements are the same are different.

  for (let srcY = 0; srcY < srcHeight; ++srcY) {
    const srcX = 0

    // Read initial values at the beginning of a row, dealing with clamping
    let A = src(srcX - 1, srcY - 1); let B = src(srcX, srcY - 1); let C = src(srcX + 1, srcY - 1)
    let D = src(srcX - 1, srcY); let E = src(srcX, srcY); let F = src(srcX + 1, srcY)
    let G = src(srcX - 1, srcY + 1); let H = src(srcX, srcY + 1); let I = src(srcX + 1, srcY + 1)

    let Q = src(srcX - 2, srcY); let R = src(srcX + 2, srcY)

    for (let srcX = 0; srcX < srcWidth; ++srcX) {
      //    .
      //    P
      //   ABC
      // .QDEFR.
      //   GHI
      //    S
      //    .

      // Default output to nearest neighbor interpolation, with
      // all four outputs equal to the center input pixel.
      let J = E; let K = E
      let L = E; let M = E

      // Skip constant 3x3 centers and just use nearest-neighbor
      // them.  This gives a good speedup on spritesheets with
      // lots of padding and full screen images with large
      // constant regions such as skies.

      // if (! all_eq8(E, A,B,C, D,F, G,H,I)) {
      // Optimized by inlining:
      if (((A ^ E) | (B ^ E) | (C ^ E) | (D ^ E) | (F ^ E) | (G ^ E) | (H ^ E) | (I ^ E)) !== 0) {
        // Read P and S, the vertical extremes that are not passed between iterations
        const P = src(srcX, srcY - 2); const S = src(srcX, srcY + 2)

        // Compute only the luminances required (faster than
        // passing all around between iterations).  Note that
        // these are premultiplied by alpha and biased so that
        // black has greater luminance than transparent.
        const Bl = luma(B); const Dl = luma(D); const El = luma(E); const Fl = luma(F); const Hl = luma(H)

        // Round some corners and fill in 1:1 slopes, but preserve
        // sharp right angles.
        //
        // In each expression, the left clause is from
        // EPX and the others are new. EPX
        // recognizes 1:1 single-pixel lines because it
        // applies the rounding only to the LINE, and not
        // to the background (it looks at the mirrored
        // side).  It thus fails on thick 1:1 edges
        // because it rounds *both* sides and produces an
        // aliased edge shifted by 1 dst pixel.  (This
        // also yields the mushroom-shaped arrow heads,
        // where that 1-pixel offset runs up against the
        // 2-pixel aligned end; this is an inherent
        // problem with 2X in-palette scaling.)
        //
        // The 2nd clause clauses avoid *double* diagonal
        // filling on 1:1 slopes to prevent them becoming
        // aliased again. It does this by breaking
        // symmetry ties using luminance when working with
        // thick features (it allows thin and transparent
        // features to pass always).
        //
        // The 3rd clause seeks to preserve square corners
        // by considering the center value before
        // rounding.
        //
        // The 4th clause identifies 1-pixel bumps on
        // straight lines that are darker than their
        // background, such as the tail on a pixel art
        // "4", and prevents them from being rounded. This
        // corrects for asymmetry in this case that the
        // luminance tie breaker introduced.

        // .------------ 1st ------------.      .----- 2nd ---------.      .------ 3rd -----.      .--------------- 4th -----------------------.
        // ◤
        if ((D === B && D !== H && D !== F) && (El >= Dl || E === A) && any_eq3(E, A, C, G) && ((El < Dl) || A !== D || E !== P || E !== Q)) J = D

        // ◥
        if ((B === F && B !== D && B !== H) && (El >= Bl || E === C) && any_eq3(E, A, C, I) && ((El < Bl) || C !== B || E !== P || E !== R)) K = B

        // ◣
        if ((H === D && H !== F && H !== B) && (El >= Hl || E === G) && any_eq3(E, A, G, I) && ((El < Hl) || G !== H || E !== S || E !== Q)) L = H

        // ◢
        if ((F === H && F !== B && F !== D) && (El >= Fl || E === I) && any_eq3(E, C, G, I) && ((El < Fl) || I !== H || E !== R || E !== S)) M = F
        // Clean up disconnected line intersections.
        //
        // The first clause recognizes being on the inside
        // of a diagonal corner and ensures that the "foreground"
        // has been correctly identified to avoid
        // ambiguous cases such as this:
        //
        //  o#o#
        //  oo##
        //  o#o#
        //
        // where trying to fix the center intersection of
        // either the "o" or the "#" will leave the other
        // one disconnected. This occurs, for example,
        // when a pixel-art letter "B" or "R" is next to
        // another letter on the right.
        //
        // The second clause ensures that the pattern is
        // not a notch at the edge of a checkerboard
        // dithering pattern.
        //

        // >
        //  .--------------------- 1st ------------------------.      .--------- 2nd -----------.
        if ((E !== F && all_eq4(E, C, I, D, Q) && all_eq2(F, B, H)) && (F !== src(srcX + 3, srcY))) K = M = F

        // <
        if ((E !== D && all_eq4(E, A, G, F, R) && all_eq2(D, B, H)) && (D !== src(srcX - 3, srcY))) J = L = D

        // v
        if ((E !== H && all_eq4(E, G, I, B, P) && all_eq2(H, D, F)) && (H !== src(srcX, srcY + 3))) L = M = H

        // ∧
        if ((E !== B && all_eq4(E, A, C, H, S) && all_eq2(B, D, F)) && (B !== src(srcX, srcY - 3))) J = K = B

        // Remove tips of bright triangles on dark
        // backgrounds. The luminance tie breaker for 1:1
        // pixel lines leaves these as sticking up squared
        // off, which makes bright triangles and diamonds
        // look bad.
        //
        // Extracting common subexpressions slows this down

        // ▲
        if (Bl < El && all_eq4(E, G, H, I, S) && none_eq4(E, A, D, C, F)) J = K = B

        // ▼
        if (Hl < El && all_eq4(E, A, B, C, P) && none_eq4(E, D, G, I, F)) L = M = H

        // ▶
        if (Fl < El && all_eq4(E, A, D, G, Q) && none_eq4(E, B, C, I, H)) K = M = F

        // ◀
        if (Dl < El && all_eq4(E, C, F, I, R) && none_eq4(E, B, A, G, H)) J = L = D
        // The first clause of each rule identifies a 2:1 slope line
        // of consistent color.
        //
        // The second clause verifies that the line is separated from
        // every adjacent pixel on one side and not part of a more
        // complex pattern. Common subexpressions from the second clause
        // are lifted to an outer test on pairs of rules.
        //
        // The actions taken by rules are unusual in that they extend
        // a color assigned by previous rules rather than drawing from
        // the original source image.
        //
        //
        // The comments show a diagram of the local
        // neighborhood in which letters shown with the
        // same shape and color must match each other and
        // everything else without annotation must be
        // different from the solid colored, square
        // letters.

        if (H !== B) { // Common subexpression
          // Above a 2:1 slope or -2:1 slope   ◢ ◣
          // First:
          if (H !== A && H !== E && H !== C) {
            // Second:
            //     P
            //   Ⓐ B C .
            // Q D 🄴 🅵 🆁
            //   🅶 🅷 I
            //     S
            if (all_eq3(H, G, F, R) && none_eq2(H, D, src(srcX + 2, srcY - 1))) L = M

            // Third:
            //     P
            // . A B Ⓒ
            // 🆀 🅳 🄴 F R
            //   G 🅷 🅸
            //     S
            if (all_eq3(H, I, D, Q) && none_eq2(H, F, src(srcX - 2, srcY - 1))) M = L
          }

          // Below a 2:1 (◤) or -2:1 (◥) slope (reflect the above 2:1 patterns vertically)
          if (B !== I && B !== G && B !== E) {
            //     P
            //   🅰 🅱 C
            // Q D 🄴 🅵 🆁
            //   Ⓖ H I .
            //     S
            if (all_eq3(B, A, F, R) && none_eq2(B, D, src(srcX + 2, srcY + 1))) J = K

            //     P
            //   A 🅱 🅲
            // 🆀 🅳 🄴 F R
            // . G H Ⓘ
            //     S
            if (all_eq3(B, C, D, Q) && none_eq2(B, F, src(srcX - 2, srcY + 1))) K = J
          }
        } // H !== B

        if (F !== D) { // Common subexpression
          // Right of a -1:2 (\) or -1:2 (/) slope (reflect the left 1:2 patterns horizontally)
          if (D !== I && D !== E && D !== C) {
            //     P
            //   🅰 B Ⓒ
            // Q 🅳 🄴 F R
            //   G 🅷 I
            //     🆂 .
            if (all_eq3(D, A, H, S) && none_eq2(D, B, src(srcX + 1, srcY + 2))) J = L

            //     🅿 .
            //   A 🅱 C
            // Q 🅳 🄴 F R
            //   🅶 H Ⓘ
            //     S
            if (all_eq3(D, G, B, P) && none_eq2(D, H, src(srcX + 1, srcY - 2))) L = J
          }

          // Left of a 1:2 (/) slope or -1:2 (\) slope (transpose the above 2:1 patterns)
          // Pull common none_eq subexpressions out
          if (F !== E && F !== A && F !== G) {
            //     P
            //   Ⓐ B 🅲
            // Q D 🄴 🅵 R
            //   G 🅷 I
            //   . 🆂
            if (all_eq3(F, C, H, S) && none_eq2(F, B, src(srcX - 1, srcY + 2))) K = M

            //   . 🅿
            //   A 🅱 C
            // Q D 🄴 🅵 R
            //   Ⓖ H 🅸
            //     S
            if (all_eq3(F, I, B, P) && none_eq2(F, H, src(srcX - 1, srcY - 2))) M = K
          }
        } // F !== D
      } // not constant

      // srcX * 2 + srcY * 4 * srcWidth
      let dstIndex = ((srcX + srcX) + (srcY << 2) * srcWidth) >>> 0
      dstBuffer[dstIndex] = J; ++dstIndex
      dstBuffer[dstIndex] = K; dstIndex += srcWidth + srcWidth - 1
      dstBuffer[dstIndex] = L; ++dstIndex
      dstBuffer[dstIndex] = M

      // Shift over already-read/computed values and bring in
      // the next srcX column. Note that we're reading from srcX
      // + 2 instead of srcX + 1 because this is reading ahead
      // one iteration.
      A = B; B = C; C = src(srcX + 2, srcY - 1)
      Q = D; D = E; E = F; F = R; R = src(srcX + 3, srcY)
      G = H; H = I; I = src(srcX + 2, srcY + 1)
    } // X
  } // Y
}
