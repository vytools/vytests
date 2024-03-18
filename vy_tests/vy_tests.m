classdef vy_tests
  properties
  endproperties

  methods
    function rslt = scatter_(obj, label, mode, x, y)
      rslt = struct("type","scatter", "mode",mode, "name",label, "x",x, "y",y);
    endfunction

    function r = random_uniform(obj, lo, hi)
        r = lo + rand()*(hi-lo);
    endfunction

    function x = getif(obj, strct,field,value)
      if isfield(strct,field)
        x = strct.(field);
      else
        x = value;
      endif
    endfunction

    function success = grade_check(obj, actual, expctd, tol)
      success = false;
      try
        if isstruct(actual) && isstruct(expctd)
          success = true;
          fn = fieldnames(actual);
          for k=1:numel(fn)
            success &= isfield(expctd,fn{k}) && obj.grade_check(actual.(fn{k}), expctd.(fn{k}), tol);
          endfor
        elseif iscell(actual) && iscell(expctd) && length(actual) == length(expctd)
          for ii = 1:length(actual)
            success &= obj.grade_check(actual{ii}, expctd{ii}, tol);
          endfor
        elseif isnumeric(actual) && isnumeric(expctd)
          if tol > 0
            success = all(abs(actual-expctd) <= tol);
          elseif actual == expctd
            success = true;
          endif
        elseif class(actual) == class(expctd)
          success = actual == expctd;
        endif
      catch ME
        ME
        success = false;
      end
    endfunction
    
    function outputs = grade_problem(obj, outputs, output, actual)
      # {"actual":, "expected":, "tolerance":, "points_possible":, "points_earned":}
      success = false;
      if isfield(outputs,output)
        outputs.(output).actual = actual;
        if ~isfield(outputs.(output),'points_possible')
          outputs.(output).points_possible = 0.0;
        end
        tolerance = obj.getif(outputs.(output), "tolerance", 0.0);
        if isfield(outputs.(output),'expected')
          expected = obj.getif(outputs.(output), "expected", actual);
          success = obj.grade_check(actual,expected,tolerance);
        end
        outputs.(output).points_earned = 0;
        if success
          outputs.(output).points_earned = outputs.(output).points_possible;
        end
      end
    endfunction

    function x = new_scatter_plot(obj, title, xlabel, ylabel, padding_left, padding_right)
      x = struct(
        "data", [],
        "layout", struct(
          "autosize", true,
          "title", title,
          "xlabel", struct("title",xlabel),
          "ylabel", struct("title",ylabel),
          "margin", struct("l",padding_left, "r",padding_right)
        ),
        "config", struct("responsive",true)
      );
    endfunction

    function data = sample(obj, xlo, xhi, n, f)
      niters = max(2,n);
      data = zeros(2,n);
      for ii = 1:niters
        x = xlo + ii/(niters-1)*(xhi-xlo);
        data(1,ii) = x;
        data(2,ii) = f(x);
      end
    endfunction

    function result = scatter_dataseries(obj, label, color, line_width, marker_size, xy)
      fi = @(varargin)varargin{length(varargin)-varargin{1}};
      n = size(xy);
      if (n(2) == 2 && n(1) ~= 2)
        xy = xy';
        n = size(xy);
      end
      if n(2) == 0 || n(1) ~= 2
        x = [];
        y = [];
      elseif n(2) == 1
        x = [xy(1,:),xy(1,:)]; # can't get a single element array for some reason, double the items
        y = [xy(2,:),xy(2,:)]; # can't get a single element array for some reason, double the items
      else 
        x = xy(1,:);
        y = xy(2,:);
      endif
      mode = fi(line_width==0,"markers",fi(marker_size==0,"lines", "lines+markers"));
      result = obj.scatter_(label, mode, x, y);
      if marker_size ~= 0
        result.marker = struct("size",marker_size);
        if ~strcmp(color, "")
          result.marker.color = color;
        endif
      endif
      if line_width ~= 0
        result.line = struct("width",line_width);
        if ~strcmp(color, "")
          result.line.color = color;
        endif
      endif
    endfunction

    function color = hsl(obj, h, s, l)
      color = sprintf("hsl( %0.1f, %0.1f, %0.1f%s)",h,s,l,"%");
    endfunction
  endmethods
end


# clear classes; vy=vy_tests(); vy.grade_check(struct('a',1,'b',[1,2;3,4]),struct('a',1,'b',[1,2;3.02,4]),0.1)